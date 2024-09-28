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
		return centerDistSquare <= rSumSquare;
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

	Window::Resize(1600, 1000);
	// Window::SetStyle(WindowStyle::Frameless);

	std::random_device seed;
	std::mt19937_64 rng(seed());

	std::vector<CircleState> circles;

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


	while (System::Update())
	{
		// 1 秒間に何回メインループが実行されているかを取得する
		int32 fps = Profiler::FPS();

		// 1 秒間に何回メインループが実行されているかを、ウィンドウタイトルに表示する
		Window::SetTitle(fps);

		double energy = 0;
		for (int i = 0; i < n; i++) {
			auto &c = circles[i];

			// ただ反転させるのではなく，このように絶対値をとれば，多少めりこんだとしても
			// 「めり込み続ける」という状態にはならないはず．
			if (c.pos.x - c.r < 0) c.v.x = abs(c.v.x); 
			if (c.pos.y - c.r < 0) c.v.y = abs(c.v.y);
			if (c.pos.x + c.r > Scene::Width()) c.v.x = -abs(c.v.x);
			if (c.pos.y + c.r > Scene::Height()) c.v.y = -abs(c.v.y);

			for (int j = i + 1; j < n; j++) {
				auto &c_ = circles[j];
				if (c.CheckCollision(c_)) { 
					double overlap = (c.r + c_.r) - (c_.pos - c.pos).length();
					// 反発処理: 位置を修正
					double angle = atan2(c_.pos.y - c.pos.y, c_.pos.x - c.pos.x);
					c.pos  -= overlap / 2 * Vec2{ cos(angle), sin(angle) };
					c_.pos += overlap / 2 * Vec2{ cos(angle), sin(angle) };

					Vec2 normal = (c_.pos - c.pos).normalized();
					Vec2 relativeVelocity = c.v - c_.v;
					double velocityAlongNormal = relativeVelocity.dot(normal);
					Vec2 impulse = velocityAlongNormal * normal;
					c.v  -= impulse;
					c_.v += impulse;
				}
			}

			c.pos.x += (Scene::DeltaTime() * c.v.x);
			c.pos.y += (Scene::DeltaTime() * c.v.y);
			Circle{ c.pos, c.r }.draw(ColorF{ 0.25 });

			energy += c.v.lengthSq();
		}
		std::cerr << energy << "\n";
	}

}