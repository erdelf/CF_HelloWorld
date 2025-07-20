#include <cute.h>
#include <cmath>
#include <cstdio>

struct Circle
{
    float radius, thickness;
	CF_Color color = cf_color_white();
};

struct Game
{
    dyna Circle* circles;
};

static void init(void* udata)
{
    Game* game = (Game*)udata;

    dyna Circle* circles = game->circles;

    cf_array_clear(circles);
    for (int i = 0; i < 10; ++i)
    {
        Circle circle ;
        circle.radius = 50 + i * 20;
        circle.thickness = 10;

		circle.color = cf_hsv_to_rgb(CF_Color(.1f * (float)i, .1f * (float)i, .1f * (float)i, 1.0f));

        cf_array_push(game->circles, circle);
	}
}

static void fixedUpdate(void* udata)
{
    Game* game = (Game*) udata;

	dyna Circle* circles = game->circles;

    for (int i = 0; i < cf_array_count(circles); ++i)
    {
        Circle* circle = circles + i;
		circle->thickness += std::sin(CF_TICKS * 10.0f + i) * 2;
    }
    /*
    update_blocks(game);
    update_stars(game);
    update_players(game);
    update_star_particles(game);

    if (cf_key_just_pressed(CF_KEY_R))
    {
        load_level(game);
    }*/
}


static void update(void* udata)
{
    Game* game = (Game*)udata;

    dyna Circle* circles = game->circles;

    for (int i = 0; i < cf_array_count(circles); ++i)
    {
        Circle* circle = circles + i;
        cf_draw_push_color(circle->color);
		cf_draw_circle2(V2(0, 0), circle->radius, circle->thickness);
		cf_draw_pop_color();
    }
}

int main(int argc, char* argv[])
{
    CF_Result result = cf_make_app("MainWindow", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
    if (cf_is_error(result))
        return -1;

    cf_set_target_framerate(60);
    cf_set_fixed_timestep(10);
    cf_clear_color(0.1f, 0.1f, 0.1f, 1.f);


	Game game = {};

    cf_set_update_udata(&game);

    init(&game);

    while (cf_app_is_running())
    {

        cf_app_update(fixedUpdate);

        update(&game);

        cf_app_draw_onto_screen(true);
    }

    cf_destroy_app();

    return 0;
}
