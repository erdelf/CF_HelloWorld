#include <cute.h>
#include <cmath>
#include <ctime>
#include <memory>
#include <vector>
#include <thread>

#include "cimgui.h"

#include <array>

/*
   hello_world namespace contains all core logic for the bouncing circles simulation,
   including object definitions, physics, rendering, and spatial partitioning.
*/
namespace hello_world
{
    // Window and world dimensions
    constexpr int kWidth = 1920;
    constexpr int kHeight = 1080;

    constexpr int kWidthHalf = kWidth / 2;
    constexpr int kHeightHalf = kHeight / 2;

    /*
        BasicObject is the base class for all objects in the simulation.
        It provides position, velocity, collision, and drawing logic.
        Derived classes should override drawInt(), update(), and fixedUpdate() as needed.
    */
    struct BasicObject
    {
        virtual ~BasicObject() = default;

        // Collision shape type and pointer to shape data
        CF_ShapeType collisionShapeType = CF_SHAPE_TYPE_NONE;
        void* collisionShape = nullptr;

        // Current position and position used for drawing (may be interpolated)
        CF_V2 position;
        CF_V2 positionDraw;

        // Indicates if the object is currently moving out of a wall after a collision
        bool movingOutOfWall = false;

        // Sets both logical and draw position
        void setPosition(const CF_V2& newPos)
        {
            position = newPos;
            positionDraw = newPos;
        }

        // Current velocity and flag for collision response
        CF_V2 velocity;
        bool velocityAdjusted;

        // Draws the object at its draw position
        void draw()
        {
            cf_draw_push();
            cf_draw_translate_v2(positionDraw);
            drawInt();
            cf_draw_pop();
        }

        // Override to implement custom drawing logic for derived objects
        virtual void drawInt()
        {
        }

        // Per-frame update
        virtual void update()
        {
            positionDraw = positionDraw + velocity * CF_DELTA_TIME;
        }

        // Fixed-timestep update (for physics and collision)
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

            // Bounce off world boundaries
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

        /*
            Tests for collision with another BasicObject.
            If a collision is detected, adjusts velocities to simulate a bounce.
        */
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

        /*
            Performs the actual collision test between two circular objects.
            Returns true if the objects are overlapping.
        */
        virtual bool collisionTestWithInt(BasicObject* other)
        {
            const float combinedShapeDistance = ((CF_Circle*)collisionShape)->r + ((CF_Circle*)other->collisionShape)->r;
            const float distance = cf_len_sq(position - other->position);
            return distance < combinedShapeDistance * combinedShapeDistance;
        }
    };

    /*
        Quadtree is a spatial partitioning structure for efficient collision detection.
        It recursively subdivides the world into quadrants to reduce the number of collision checks.
    */
    struct Quadtree {
        static constexpr int kMaxObjects = 8;
        static constexpr int kMaxLevels = 12;

        /*
            AABB (Axis-Aligned Bounding Box) defines a rectangular region in 2D space.
        */
        struct AABB {
            float x, y, hw, hh;

            // Returns true if the circle (center p, radius r) is fully contained in this AABB
            bool contains(const CF_V2& p, float r) const {
                return p.x - r >= x - hw && p.x + r <= x + hw &&
                    p.y - r >= y - hh && p.y + r <= y + hh;
            }

            // Returns true if this AABB intersects with another
            bool intersects(const AABB& other) const {
                return !(x + hw < other.x - other.hw || x - hw > other.x + other.hw ||
                    y + hh < other.y - other.hh || y - hh > other.y + other.hh);
            }
        };

        int level; // Current depth in the quadtree
        AABB bounds; // Bounding box for this node
        std::vector<BasicObject*> objects; // Objects in this node
        std::array<std::unique_ptr<Quadtree>, 4> nodes; // Child nodes

        Quadtree(const Quadtree&) = delete;
        Quadtree& operator=(const Quadtree&) = delete;

        // Constructs a quadtree node at a given level and bounds
        Quadtree(const int lvl, const AABB& b) : level(lvl), bounds(b) {}

        // Clears all objects and child nodes recursively
        void clear() {
            objects.clear();
            for (auto& n : nodes) if (n) n->clear();
            for (auto& n : nodes) if (n) n.reset();
        }

        // Splits this node into four child quadrants
        void split() {
            const float x = bounds.x, y = bounds.y, hw = bounds.hw / 2, hh = bounds.hh / 2;
            nodes[0] = std::make_unique<Quadtree>(level + 1, AABB{ .x = x + hw, .y = y - hh, .hw = hw, .hh = hh });
            nodes[1] = std::make_unique<Quadtree>(level + 1, AABB{ .x = x - hw, .y = y - hh, .hw = hw, .hh = hh });
            nodes[2] = std::make_unique<Quadtree>(level + 1, AABB{ .x = x - hw, .y = y + hh, .hw = hw, .hh = hh });
            nodes[3] = std::make_unique<Quadtree>(level + 1, AABB{ .x = x + hw, .y = y + hh, .hw = hw, .hh = hh });
        }

        /*
            Determines which child node the object belongs to.
            Returns -1 if the object cannot fit completely within a child node.
        */
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

        /*
            Inserts an object into the quadtree.
            If the node exceeds capacity, it splits and redistributes objects.
        */
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

        /*
            Retrieves all objects that could collide with the given object.
            Results are appended to the 'out' vector.
        */
        void retrieve(const BasicObject* obj, std::vector<BasicObject*>& out) const {
            const float r = ((CF_Circle*)obj->collisionShape)->r;
            const int idx = getIndex(obj->position, r);

            if (nodes[0] && idx != -1) {
                nodes[idx]->retrieve(obj, out);
            }
            out.insert(out.end(), objects.begin(), objects.end());
        }
    };

    /*
        Circle represents a drawable circle with a radius, thickness, and color.
    */
    struct Circle
    {
        float radius, thickness;
        CF_Color color = cf_color_white();

        // Draws the circle at the current transform
        void draw() const
        {
            cf_draw_push_color(color);
            cf_draw_circle2(V2(0, 0), radius, thickness);
            cf_draw_pop_color();
        }
    };

    /*
        BouncingCircle is a BasicObject that consists of one or more circles.
        It handles its own initialization, color, and drawing.
    */
    struct BouncingCircle final : BasicObject
    {
        dyna Circle* circles;

        /*
            Constructs a BouncingCircle with random position, velocity, and color.
            Sets up its collision shape and visual appearance.
        */
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

                // Color is based on position and some randomness
                const float h = (position.x + kWidthHalf) / kWidth; // Hue: [0, 1]
                const float s = 0.5f + (position.y + kHeightHalf) / (1 / .5f * kHeight); // Saturation: [0.5, 1]
                const float v = 0.7f + static_cast<float>(rand()) / (1 / .3f * RAND_MAX); // Value: [0.7, 1]

                circle.color = cf_hsv_to_rgb(CF_Color(h, s, v, 1.));
                cf_array_push(circles, circle);
            }
            velocity = V2(rand() % (kWidth / 2) * (rand() % 2 == 1 ? 1 : -1), rand() % (kHeight / 2) * (rand() % 2 == 1 ? 1 : -1));
            CF_Circle* cf_circle = new CF_Circle{ position, highestRadius };
            collisionShape = cf_circle;
        }

        // Frees memory for circles array
        ~BouncingCircle() override
        {
            cf_array_free(circles);
            circles = nullptr;
        }

        // Draws all circles in the object
        void drawInt() override
        {
            for (int c = 0; c < cf_array_count(circles); ++c)
                (circles + c)->draw();
        }
    };

    /*
        Game holds all simulation objects.
        It is passed as user data to update and render functions.
    */
    struct Game
    {
        ~Game()
        {
            objects.clear();
        }

        std::vector<std::unique_ptr<BasicObject>> objects;
    };

    /*
        Initializes the game by creating a large number of BouncingCircle objects.
        Called once at startup.
    */
    static void init(void* udata)
    {
        Game* game = (Game*)udata;
        std::vector<std::unique_ptr<BasicObject>> objects;
        for (int i = 0; i < 5000; ++i)
            objects.push_back(std::make_unique<BouncingCircle>());
        game->objects = std::move(objects);
    }

    // World bounds for the quadtree
    constexpr Quadtree::AABB worldAABB{ .x = 0, .y = 0, .hw = kWidthHalf, .hh = kHeightHalf };

    /*
        Performs fixed-timestep updates for all objects, including physics and collision.
        Uses a quadtree for efficient broad-phase collision detection.
    */
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

    /*
        Performs per-frame updates and draws all objects.
        Called once per frame.
    */
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

/*
    Entry point for the application.
    Sets up the window, initializes the game, and runs the main loop.
*/
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