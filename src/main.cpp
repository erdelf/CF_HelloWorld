#include <cute.h>
#include <cmath>
#include <ctime>
#include <memory>
#include <vector>
#include <thread>

#include "cimgui.h"


#include <array>

namespace hello_world
{
    constexpr int kWidth = 1920;
    constexpr int kHeight = 1080;

    constexpr int kWidthHalf = kWidth / 2;
    constexpr int kHeightHalf = kHeight / 2;

    struct BasicObject
    {
        virtual ~BasicObject() = default;
        CF_ShapeType collisionShapeType = CF_SHAPE_TYPE_NONE;
        void* collisionShape = nullptr;

        CF_V2 position;
        CF_V2 positionDraw;

        bool movingOutOfWall = false;

        void setPosition(const CF_V2& newPos)
        {
            position = newPos;
            positionDraw = newPos;
        }

        CF_V2 velocity;
        bool velocityAdjusted;

        void draw()
        {
            cf_draw_push();
            cf_draw_translate_v2(positionDraw);
            drawInt();
            cf_draw_pop();
        }

        virtual void drawInt()
        {
        }

        virtual void update()
        {
            positionDraw = positionDraw + velocity * CF_DELTA_TIME;
        }

        virtual void fixedUpdate()
        {
            velocityAdjusted = false;

            const CF_V2 pos = position + velocity * CF_DELTA_TIME_FIXED;
            setPosition(pos);

            if (movingOutOfWall)
            {
                bool done = false;

                if (pos.x < -kWidthHalf && velocity.x < 0)
                    velocity.x = abs(velocity.x);
                else if (pos.x > kWidthHalf && velocity.x > 0)
                    velocity.x = abs(velocity.x) * -1;
                else
                    done = true;

                if (pos.y < -kHeightHalf && velocity.y < 0)
                    velocity.y = abs(velocity.y);
                else if (pos.y > kHeightHalf && velocity.y > 0)
                    velocity.y = abs(velocity.y) * -1;
                else if (done)
                {
                    movingOutOfWall = false;
                    return;
                }
                return;
            }

            // Bounce off walls
            if (pos.x < -kWidthHalf || pos.x > kWidthHalf)
            {
                velocity.x = -velocity.x;
                movingOutOfWall = true;
            }

            if (pos.y < -kHeightHalf || pos.y > kHeightHalf)
            {
                velocity.y = -velocity.y;
                movingOutOfWall = true;
            }
        }

        virtual void collisionTestWith(BasicObject* other)
        {
            if (other == nullptr || other == this || collisionShapeType == CF_SHAPE_TYPE_NONE || other->collisionShapeType == CF_SHAPE_TYPE_NONE || (velocityAdjusted && other->velocityAdjusted))
                return;

            if (collisionTestWithInt(other))
            {
                CF_V2 velNew = position - other->position;
                velNew = velNew / cf_len(velNew);

                if (!velocityAdjusted)
					velocity = velNew * cf_len(velocity);
                if (!other->velocityAdjusted)
					other->velocity = -velNew * cf_len(other->velocity);

				velocityAdjusted = true;
                other->velocityAdjusted = true;
            }
        }

        virtual bool collisionTestWithInt(BasicObject* other)
        {
            const float combinedShapeDistance = ((CF_Circle*)collisionShape)->r + ((CF_Circle*)other->collisionShape)->r;
            const float distance = cf_len_sq(position - other->position);
            return distance < combinedShapeDistance * combinedShapeDistance;
        }
    };

    struct Quadtree {
        static constexpr int kMaxObjects = 8;
        static constexpr int kMaxLevels = 12;

        struct AABB {
            float x, y, hw, hh;

        	bool contains(const CF_V2& p, float r) const {
                return p.x - r >= x - hw && p.x + r <= x + hw &&
                    p.y - r >= y - hh && p.y + r <= y + hh;
            }

        	bool intersects(const AABB& other) const {
                return !(x + hw < other.x - other.hw || x - hw > other.x + other.hw ||
                    y + hh < other.y - other.hh || y - hh > other.y + other.hh);
            }
        };

        int level;
        AABB bounds;
        std::vector<BasicObject*> objects;
        std::array<std::unique_ptr<Quadtree>, 4> nodes;

        Quadtree(const Quadtree&) = delete;
        Quadtree& operator=(const Quadtree&) = delete;

        Quadtree(const int lvl, const AABB& b) : level(lvl), bounds(b) {}

        void clear() {
            objects.clear();
            for (auto& n : nodes) if (n) n->clear();
            for (auto& n : nodes) if (n) n.reset();
        }

        void split() {
            const float x = bounds.x, y = bounds.y, hw = bounds.hw / 2, hh = bounds.hh / 2;
            nodes[0] = std::make_unique<Quadtree>(level + 1, AABB{ .x= x + hw, .y= y - hh, .hw= hw, .hh= hh});
            nodes[1] = std::make_unique<Quadtree>(level + 1, AABB{ .x= x - hw, .y= y - hh, .hw= hw, .hh= hh});
            nodes[2] = std::make_unique<Quadtree>(level + 1, AABB{ .x= x - hw, .y= y + hh, .hw= hw, .hh= hh});
            nodes[3] = std::make_unique<Quadtree>(level + 1, AABB{ .x= x + hw, .y= y + hh, .hw= hw, .hh= hh});
        }

        int getIndex(const CF_V2& pos, const float r) const {
            const bool top = pos.y - r < bounds.y && pos.y + r < bounds.y;
            const bool bottom = pos.y - r > bounds.y;
            const bool left = pos.x - r < bounds.x && pos.x + r < bounds.x;
            const bool right = pos.x - r > bounds.x;
            if (top) {
                if (right) return 0;
                if (left) return 1;
            }
            else if (bottom) {
                if (left) return 2;
                if (right) return 3;
            }
            return -1;
        }

        void insert(BasicObject* obj) {
            const float r = ((CF_Circle*)obj->collisionShape)->r;
            if (nodes[0]) {
	            if (const int idx = getIndex(obj->position, r); 
                    idx != -1) {
                    nodes[idx]->insert(obj);
                    return;
                }
            }
            objects.push_back(obj);
            if (objects.size() > kMaxObjects && level < kMaxLevels) {
                if (!nodes[0]) 
                    split();
                auto it = objects.begin();
                while (it != objects.end()) {
                    const float r2 = ((CF_Circle*)(*it)->collisionShape)->r;
                    if (const int idx = getIndex((*it)->position, r2); 
                        idx != -1) {
                        nodes[idx]->insert(*it);
                        it = objects.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
        }

        void retrieve(const BasicObject* obj, std::vector<BasicObject*>& out) const {
            const float r = ((CF_Circle*)obj->collisionShape)->r;
            const int idx = getIndex(obj->position, r);

            if (nodes[0] && idx != -1) {
                nodes[idx]->retrieve(obj, out);
            }
            out.insert(out.end(), objects.begin(), objects.end());
        }
    };


    struct Circle
    {
        float radius, thickness;
        CF_Color color = cf_color_white();

        void draw() const
        {
            cf_draw_push_color(color);
            cf_draw_circle2(V2(0, 0), radius, thickness);
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
            const int circleCount = rand() % 1 + 1;
            float highestRadius = 0.f;

            position = V2(rand() % kWidth - kWidthHalf, rand() % kHeight - kHeightHalf);

            for (int c = 0; c < circleCount; c++)
            {
                Circle circle;
                circle.radius = 1.f + static_cast<float>(c) * 10.f;
                circle.thickness = 5;
                highestRadius = circle.radius + 10;

                /*
                const float h = static_cast<float>(rand()) / RAND_MAX; // Hue: [0, 1]
                const float s = 0.5f + static_cast<float>(rand()) / (1 / .5f * RAND_MAX); // Saturation: [0.5, 1]
                const float v = 0.7f + static_cast<float>(rand()) / (1 / .3f * RAND_MAX); // Value: [0.7, 1]
				*/
                const float h = (position.x + kWidthHalf) / kWidth; // Hue: [0, 1]
                const float s = 0.5f + (position.y + kHeightHalf) / (1 / .5f * kHeight); // Saturation: [0.5, 1]
                const float v = 0.7f + static_cast<float>(rand()) / (1 / .3f * RAND_MAX); // Value: [0.7, 1]


                circle.color = cf_hsv_to_rgb(CF_Color(h, s, v, 1.));
                cf_array_push(circles, circle);
            }
            velocity = V2(rand() % (kWidth / 2) * (rand() % 2 == 1 ? 1 : -1), rand() % (kHeight / 2) * (rand() % 2 == 1 ? 1 : -1));
            CF_Circle* cf_circle = new CF_Circle { position, highestRadius };
            collisionShape = cf_circle;
        }

        ~BouncingCircle() override
        {
            cf_array_free(circles);
            circles = nullptr;
        }

        void drawInt() override
        {
            for (int c = 0; c < cf_array_count(circles); ++c)
                (circles + c)->draw();
        }
    };

    struct Game
    {
        ~Game()
        {
            objects.clear();
        }

        std::vector<std::unique_ptr<BasicObject>> objects;
    };

    static void init(void* udata)
    {
        Game* game = (Game*)udata;
        std::vector<std::unique_ptr<BasicObject>> objects;
        for (int i = 0; i < 5000; ++i)
            objects.push_back(std::make_unique<BouncingCircle>());
        game->objects = std::move(objects);
    }

    constexpr Quadtree::AABB worldAABB{ .x = 0, .y = 0, .hw = kWidthHalf, .hh = kHeightHalf };

    static void fixedUpdate(void* udata)
    {
        const Game* game = (Game*)udata;
        const auto& objects = game->objects;

        for (const auto& obj : objects)
            obj->fixedUpdate();

        Quadtree quadtree(0, worldAABB);

        for (const auto& obj : objects)
            quadtree.insert(obj.get());

        std::vector<BasicObject*> candidates;

        for (const auto& obj : objects) {
            candidates.clear();
            quadtree.retrieve(obj.get(), candidates);
            for (auto* other : candidates) {
                if (obj.get() != other)
                    obj->collisionTestWith(other);
            }
        }
    }

    static void update(void* udata)
    {
        const Game* game = (Game*)udata;
        const auto& objects = game->objects;

        for (const auto& obj : objects)
        {
			obj->update();
	        obj->draw();
        }
    }
}

int main(int argc, char* argv[])
{
    if (const CF_Result result = cf_make_app("MainWindow", 0, 0, 0, hello_world::kWidth, hello_world::kHeight, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]); cf_is_error(result))
        return -1;
    
    cf_set_target_framerate(60);
    cf_set_fixed_timestep(10);
    cf_clear_color(0.1f, 0.1f, 0.1f, 1.f);

    srand((unsigned)time(nullptr));

	hello_world::Game game;

    cf_set_update_udata(&game);
    init(&game);

    cf_app_init_imgui();

    static bool debugMenu = true;

    cf_pause_for(10);

    while (cf_app_is_running())
    {
        cf_app_update(hello_world::fixedUpdate);

        if (debugMenu)
        {
            igBegin("Debug", &debugMenu, 0);
            igText("FPS: %f", cf_app_get_framerate());
            igEnd();
        }

        update(&game);

        cf_app_draw_onto_screen(true);
    }

    cf_destroy_app();

    return 0;
}