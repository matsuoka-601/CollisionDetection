#include <Siv3D.hpp>
#include <random>

struct CircleState {
	Vec2 pos;
	Vec2 v;
	int r;

	CircleState (Vec2 pos_, Vec2 v_, int r_) 
		: pos(pos_), v(v_), r(r_) {}

	bool CheckCollision(CircleState &target) {
		double centerDistSquare = (pos - target.pos).lengthSq();
		double rSumSquare = (r + target.r) * (r + target.r);
		return centerDistSquare < rSumSquare;
	}
};

#define GridRowCount 64
#define GridColCount 64

std::vector<std::pair<int, int>> IntersectGridSet(CircleState &c) {
	int gridHeight = (Scene::Height() + GridRowCount - 1) / GridRowCount;
	int gridWidth = (Scene::Width() + GridColCount - 1) / GridColCount;

	std::vector<std::pair<int, int>> ret;

	float xmin = c.pos.x - c.r;
	float xmax = c.pos.x + c.r;
	float ymin = c.pos.y - c.r;
	float ymax = c.pos.y + c.r;

	for (int k = ymin / gridHeight; k <= ymax / gridHeight; k++) 
		for (int l = xmin / gridWidth; l <= xmax / gridWidth; l++) 
			ret.push_back({k, l});

	return ret;
}

void Main()
{
	Scene::SetBackground(Palette::White);

    	const double speed = 180.0; // 毎秒 180px 移動
	const int r = 3;


	std::random_device seed;
	std::mt19937_64 rng(seed());

	std::vector<CircleState> circles;

	Window::Resize(1600, 1000);

	const int n = 15000;
	for (int i = 0; i < n; i++) {
		// 既に作った球と衝突しない球ができるまでループ
		while (true) {
			double x = (rng() % (Scene::Width() - 2 * r)) + r;
			double y = (rng() % (Scene::Height() - 2 * r)) + r;
			int direction = rng();
			double vx = speed * cos(direction);
			double vy = speed * sin(direction);
			CircleState c(Vec2{x, y}, Vec2{vx, vy}, r);
			bool ok = true;
			for (int j = 0; j < i; j++) 
				if (c.CheckCollision(circles[j])) 
					ok = false;
			if (ok) {
				circles.push_back(c);
				break;
			}
		}
	}

	// たまに壁をすり抜けるので番兵を設けておく
	std::vector<int> grid[GridRowCount + 1][GridColCount + 1];

	for (int i = 0; i < n; i++) {
		auto &c = circles[i];
		auto intersectGridSet = IntersectGridSet(c); // この球と共有点を持つグリッド
		for (auto [gridRow, gridCol]: intersectGridSet) {
			grid[gridRow][gridCol].push_back(i);
		}
	}


	while (System::Update())
	{
		// 1 秒間に何回メインループが実行されているかを取得する
		int32 fps = Profiler::FPS();

		// 1 秒間に何回メインループが実行されているかを、ウィンドウタイトルに表示する
		Window::SetTitle(fps);

		for (int i = 0; i < n; i++) {
			auto &c = circles[i];

			// ただ反転させるのではなく，このように絶対値をとれば，多少めりこんだとしても
			// 「めり込み続ける」という状態にはならないはず．
			if (c.pos.x - c.r < 0) c.v.x = abs(c.v.x); 
			if (c.pos.y - c.r < 0) c.v.y = abs(c.v.y);
			if (c.pos.x + c.r > Scene::Width()) c.v.x = -abs(c.v.x);
			if (c.pos.y + c.r > Scene::Height()) c.v.y = -abs(c.v.y);

			auto intersectGridSet = IntersectGridSet(c); // この球と共有点を持つグリッド

			// 懸念 : 相手が複数のグリッドに属している場合，何回も当たり判定が起こるんじゃないか？
            		for (auto [gridRow, gridCol]: intersectGridSet) {
				for (auto &idx: grid[gridRow][gridCol]) {
				    auto &c_ = circles[idx];
				    if (i < idx && c.CheckCollision(c_)) { 
					// めり込まないように反発を入れる
					double overlap = (c.r + c_.r) - (c_.pos - c.pos).length();
					double angle = atan2(c_.pos.y - c.pos.y, c_.pos.x - c.pos.x);
					c.pos  -= overlap * Vec2{ cos(angle), sin(angle) };
					c_.pos += overlap * Vec2{ cos(angle), sin(angle) };
		
					Vec2 normal = (c_.pos - c.pos).normalized();
					Vec2 relativeVelocity = c.v - c_.v;
					double velocityAlongNormal = relativeVelocity.dot(normal);
					Vec2 impulse = velocityAlongNormal * normal;
					c.v  -= impulse;
					c_.v += impulse;
			    		}	
				}
			}
        	}

		for (int gridRow = 0; gridRow < GridRowCount; gridRow++)
			for (int gridCol = 0; gridCol < GridColCount; gridCol++)
				grid[gridRow][gridCol].clear();

		float energy = 0;
		for (int i = 0; i < n; i++) {
			auto &c = circles[i];
			c.pos += Scene::DeltaTime() * c.v; // 位置を更新
			Circle{ c.pos, c.r }.draw(ColorF{ 0.25 });

			auto intersectGridSet = IntersectGridSet(c); // この球と共有点を持つグリッド
			for (auto [gridRow, gridCol]: intersectGridSet) {
				if (gridRow >= GridRowCount + 1 || gridCol >= GridColCount + 1) {
					std::cerr << gridRow << " " << gridCol << "\n" ;
					std::cerr << c.pos.y << " " << c.pos.x << "\n" ;
					exit(0);
				}
				grid[gridRow][gridCol].push_back(i);
			}

			energy += c.v.lengthSq();
		}
		std::cerr << energy << "\n"; // エネルギー保存則が成り立つので，この値は一定
    	}
}
