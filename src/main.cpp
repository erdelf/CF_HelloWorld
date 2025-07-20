#include <cute.h>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

constexpr int width = 640;
constexpr int height = 480;

constexpr int widthHalf = width/2;
constexpr int heightHalf = height / 2;

struct Circle
{
    float radius, thickness;
	CF_Color color = cf_color_white();
};

struct BouncingCircle
{
    dyna Circle* circles;
    CF_V2 position;
    CF_V2 positionDraw;
    CF_V2 velocity;
};

struct Game
{
    dyna BouncingCircle* circles;
};

static void init(void* udata)
{
    const Game* game = (Game*)udata;



    dyna BouncingCircle* circles = game->circles;
    cf_array_clear(circles);

    for (int i = 0; i < 10; ++i)
    {
        BouncingCircle bCircle = {};

        const int circleCount = rand() % 5;
        for (int c = 0; c < circleCount; c++)
        {
            Circle circle;
            circle.radius = 1.f + static_cast<float>(c) * 10.f;
            circle.thickness = 5;


            const float h = static_cast<float>(rand()) / RAND_MAX; // Hue: [0, 1]
            const float s = 0.5f + static_cast<float>(rand()) / (1/.5f * RAND_MAX); // Saturation: [0.5, 1]
            const float v = 0.7f + static_cast<float>(rand()) / (1/.3f * RAND_MAX); // Value: [0.7, 1]

            circle.color = cf_hsv_to_rgb(CF_Color(h, s, v, 1.));

            cf_array_push(bCircle.circles, circle);
        }
		bCircle.position = V2(rand() % width - widthHalf, rand() % height - heightHalf);
		bCircle.velocity = V2(rand() % (width/3) * (rand() % 2 == 1 ? 1 : -1), rand() % (height / 3) * (rand() % 2 == 1 ? 1 : -1));

        cf_array_push(game->circles, bCircle);
	}
}

static void fixed_update(void* udata)
{
    const Game* game = (Game*) udata;

	dyna BouncingCircle* circles = game->circles;

    for (int i = 0; i < cf_array_count(circles); ++i)
    {
        BouncingCircle* circle = circles + i;
		circle->position += circle->velocity * CF_DELTA_TIME_FIXED;

        // Bounce off walls
        if (circle->position.x < -widthHalf || circle->position.x > widthHalf)
        {
            circle->velocity.x = -circle->velocity.x;
            circle->position.x = cf_clamp(circle->position.x, -widthHalf, widthHalf);
        }
        if (circle->position.y < -heightHalf || circle->position.y > heightHalf)
        {
            circle->velocity.y = -circle->velocity.y;
            circle->position.y = cf_clamp(circle->position.y, -heightHalf, heightHalf);
		}

        circle->positionDraw = circle->position;
    }
}


static void update(void* udata)
{
    const Game* game = (Game*)udata;

    dyna BouncingCircle* circles = game->circles;

    for (int i = 0; i < cf_array_count(circles); ++i)
    {
		BouncingCircle* bCircle = circles + i;

        cf_draw_push();
		bCircle->positionDraw = bCircle->positionDraw + bCircle->velocity * CF_DELTA_TIME;
        cf_draw_translate_v2(bCircle->positionDraw);
        for (int c = 0; c < cf_array_count(bCircle->circles); ++c)
        {
            const Circle* circle = bCircle->circles + c;
            cf_draw_push_color(circle->color);
            cf_draw_circle2(V2(0, 0), circle->radius, circle->thickness);
            cf_draw_pop_color();
        }
        cf_draw_pop();
    }
}

int main(int argc, char* argv[])
{
	if (const CF_Result result = cf_make_app("MainWindow", 0, 0, 0, width, height, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]); cf_is_error(result))
        return -1;

    cf_set_target_framerate(60);
    cf_set_fixed_timestep(10);
    cf_clear_color(0.1f, 0.1f, 0.1f, 1.f);

    srand((unsigned) time(nullptr));

	Game game = {};

    cf_set_update_udata(&game);

    init(&game);

    while (cf_app_is_running())
    {

        cf_app_update(fixed_update);

        update(&game);

        cf_app_draw_onto_screen(true);
    }

    cf_destroy_app();

    return 0;
}
