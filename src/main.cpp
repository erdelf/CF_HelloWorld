#include <cute.h>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

constexpr int width = 640;
constexpr int height = 480;

constexpr int widthHalf = width/2;
constexpr int heightHalf = height / 2;

struct BasicObject
{
	virtual ~BasicObject() = default;
	CF_ShapeType collisionShapeType = CF_SHAPE_TYPE_NONE;
	void* collisionShape = nullptr;

    CF_V2 position;
	CF_V2 positionDraw;

	void set_position(const CF_V2& new_pos)
	{
		this->position = new_pos;
		this->positionDraw = new_pos;
	}


	CF_V2 velocity;

    void Draw()
    {
        cf_draw_push();
        cf_draw_translate_v2(positionDraw);
		DrawInt();
		cf_draw_pop();
    }

    virtual void DrawInt()
	{}

	virtual void update()
	{
        positionDraw = positionDraw + velocity * CF_DELTA_TIME;
	}

	virtual void fixed_update()
	{
        const CF_V2 pos = position + velocity * CF_DELTA_TIME_FIXED;
        set_position(pos);

        // Bounce off walls
        if (pos.x < -widthHalf || pos.x > widthHalf)
	        velocity.x = -velocity.x;
        if (pos.y < -heightHalf || pos.y > heightHalf)
	        velocity.y = -velocity.y;
	}

	virtual void CollisionTestWith(BasicObject* other)
    {
        if (other == nullptr || other == this || collisionShapeType == CF_SHAPE_TYPE_NONE || other->collisionShapeType == CF_SHAPE_TYPE_NONE)
			return;


        /**
		 *  manual works but currently hardcoded for circles
        if (cf_len((position - other->position)) < (((CF_Circle*)(collisionShape))->r + ((CF_Circle*)(other->collisionShape))->r))
        {
            printf("collision detected\n");
		}
		*/
        
        /**
			Seemingly none of the cf collision functions work, so this is commented out for now.

        CF_Transform* transform = new CF_Transform();
        transform->r = cf_sincos();
		transform->p = position;

        CF_Transform* transformOther = new CF_Transform();
        transformOther->r = cf_sincos();
        transformOther->p = other->position;

        if (cf_circle_to_circle(*((CF_Circle*)(collisionShape)), *((CF_Circle*)(other->collisionShape))))
	        printf("circle to circle collision detected\n");

        if (cf_collided(collisionShape, transform, collisionShapeType, other->collisionShape, transformOther, other->collisionShapeType))
	    {
            printf("collided\n");

			const CF_V2 vel_new = position - positionDraw;

			velocity = vel_new * cf_len(velocity);
			other->velocity = -vel_new * cf_len(other->velocity);

            fixed_update();
		}
		*/
    }
};


struct Circle
{
    float radius, thickness;
	CF_Color color = cf_color_white();

    void Draw() const
    {
        cf_draw_push_color(color);
        cf_draw_circle2(V2(0,0), radius, thickness);
        cf_draw_pop_color();
	}
};

struct BouncingCircle final : BasicObject
{
    dyna Circle* circles;

	BouncingCircle()
	{
		collisionShapeType = CF_SHAPE_TYPE_CIRCLE;

        circles = {};

        const int circleCount = rand() % 5 + 1;

        float highestRadius = 0.f;

        for (int c = 0; c < circleCount; c++)
        {
            Circle circle;
            circle.radius = 1.f + static_cast<float>(c) * 10.f;
            circle.thickness = 5;

            highestRadius = circle.radius;


            const float h = static_cast<float>(rand()) / RAND_MAX; // Hue: [0, 1]
            const float s = 0.5f + static_cast<float>(rand()) / (1 / .5f * RAND_MAX); // Saturation: [0.5, 1]
            const float v = 0.7f + static_cast<float>(rand()) / (1 / .3f * RAND_MAX); // Value: [0.7, 1]

            circle.color = cf_hsv_to_rgb(CF_Color(h, s, v, 1.));

            cf_array_push(circles, circle);
        }
        position = V2(rand() % width - widthHalf, rand() % height - heightHalf);
        velocity = V2(rand() % (width / 3) * (rand() % 2 == 1 ? 1 : -1), rand() % (height / 3) * (rand() % 2 == 1 ? 1 : -1));

        CF_Circle* circleShape = new CF_Circle{position, highestRadius};
        printf("radius: %f\n", highestRadius);
        collisionShape = static_cast<void*>(circleShape);
	}

	~BouncingCircle() override
	{
        cf_array_free(circles);
		circles = nullptr;
	}

    void DrawInt() override
    {
        for (int c = 0; c < cf_array_count(circles); ++c)
	        (circles + c)->Draw();
    }
};

struct Game
{
	~Game()
	{
        objects.clear();
    }

	std::vector<BasicObject*> objects;
};

static void init(void* udata)
{
	Game* game = (Game*)udata;

    std::vector<BasicObject*> objects = {};

    for (int i = 0; i < 5; ++i)
    {
        BouncingCircle* bCircle = new BouncingCircle();
        objects.push_back(bCircle);
	}

	game->objects = objects;
}

static void fixed_update(void* udata)
{
    const Game* game = (Game*) udata;

    const std::vector<BasicObject*> objects = game->objects;

    for (BasicObject* obj : objects)
	    obj->fixed_update();

    for (BasicObject* objA : objects)
        for (BasicObject* objB : objects)
            objA->CollisionTestWith(objB);
}


static void update(void* udata)
{
    const Game* game = (Game*)udata;

    const std::vector<BasicObject*> objects = game->objects;

    for (BasicObject* obj : objects)
        obj->update();

    for (BasicObject* obj : objects)
        obj->Draw();
}

int main(int argc, char* argv[])
{
	if (const CF_Result result = cf_make_app("MainWindow", 0, 0, 0, width, height, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]); cf_is_error(result))
        return -1;

    cf_set_target_framerate(60);
    cf_set_fixed_timestep(10);
    cf_clear_color(0.1f, 0.1f, 0.1f, 1.f);

    srand((unsigned) time(nullptr));

	Game game = *(new Game());

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
