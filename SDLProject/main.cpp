
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 1.5f,
              WINDOW_HEIGHT = 480 * 1.5f;

constexpr float BG_RED     = 1.0f,
                BG_GREEN   = 1.0f,
                BG_BLUE    = 1.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char LEFT_SPRITE_FILEPATH[] = "leftplay.png",
               RIGHT_SPRITE_FILEPATH[] = "rightplay.png",
               BALL_SPRITE_FILEPATH[]  = "bean.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_BALL  = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_BALL    = glm::vec3(-4.0f, 2.0f, 0.0f),
                    INIT_SCALE_LEFT = glm::vec3(3.0f, 3.0f, 0.0f),
                    INIT_POS_LEFT   = glm::vec3(-3.0f, -2.0f, 0.0f),
                    INIT_SCALE_RIGHT = glm::vec3(3.0f, 3.0f, 0.0f),
                    INIT_POS_RIGHT   = glm::vec3(3.0f, -2.0f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_left_matrix, g_right_matrix, g_projection_matrix, g_ball_matrix;

float g_previous_ticks = 0.0f;

GLuint g_left_texture_id;
GLuint g_right_texture_id;
GLuint g_ball_texture_id;

constexpr float MOVING_SPEED = 2.0f;

glm::vec3 g_left_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_left_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_right_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_right_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);


glm::vec3 g_ball_velocity = glm::vec3(2.0f, -0.02f, 0.0f);
glm::vec3 g_gravity = glm::vec3(0.3f, -0.09f, 0.0f);

bool self = false;
glm::vec3 g_self_velocity = glm::vec3(0.0f, -0.02f, 0.0f);

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_left_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_ball_position += g_ball_movement;

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_left_texture_id = load_texture(LEFT_SPRITE_FILEPATH);
    g_right_texture_id = load_texture(RIGHT_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    g_left_movement = glm::vec3(0.0f);
    g_right_movement = glm::vec3(0.0f);
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q: g_app_status = TERMINATED; break;
                        
                    case SDL_SCANCODE_T:
                        self = !self;
                        break;
                        
                    case SDL_SCANCODE_S:
                        g_left_movement.y = -1.0f;
                        break;
                        
                    case SDL_SCANCODE_W:
                        g_left_movement.y = 1.0f;
                        break;
                        
                    case SDL_SCANCODE_DOWN:
                        g_right_movement.y = -1.0f;
                        break;
                        
                    case SDL_SCANCODE_UP:
                        g_right_movement.y = 1.0f;
                        break;
                        
                    default: break;
                }
                
            default:
                break;
        }
    }
    
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (key_state[SDL_SCANCODE_T]){
        self = !self;
    }
    if (!self){
        if (key_state[SDL_SCANCODE_S]){
            g_left_movement.y = -1.0f;
        }
        if (key_state[SDL_SCANCODE_W]){
            g_left_movement.y = 1.0f;
        }
    }
    if (key_state[SDL_SCANCODE_DOWN]){
        g_right_movement.y = -1.0f;
    }
    if (key_state[SDL_SCANCODE_UP]){
        g_right_movement.y = 1.0f;
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    
    if (self){
        g_left_position += g_self_velocity * MOVING_SPEED;
        if ((g_left_position.y + INIT_POS_LEFT.y + INIT_SCALE_LEFT.y / 2.0f > 3.75f)||(g_left_position.y + INIT_POS_LEFT.y - INIT_SCALE_LEFT.y / 2.0f < -3.75f))  {
            g_self_velocity.y = -g_self_velocity.y;
        }
    }
    else{
        g_left_position += g_left_movement * MOVING_SPEED * delta_time;
    }
    g_right_position += g_right_movement * MOVING_SPEED * delta_time;

    g_ball_velocity.y += g_gravity.y * delta_time;

    g_ball_position += g_ball_velocity * delta_time;
//    std::cout << "Ball Position: " << g_ball_position.x << ", " << g_ball_position.y << std::endl;


    g_left_matrix = glm::mat4(1.0f);
    g_left_matrix = glm::translate(g_left_matrix, INIT_POS_LEFT);
    g_left_matrix = glm::translate(g_left_matrix, g_left_position);
    
    g_right_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::translate(g_right_matrix, INIT_POS_RIGHT);
    g_right_matrix = glm::translate(g_right_matrix, g_right_position);

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    g_left_matrix = glm::scale(g_left_matrix, INIT_SCALE_LEFT);
    g_right_matrix = glm::scale(g_right_matrix, INIT_SCALE_RIGHT);
    g_ball_matrix  = glm::scale(g_ball_matrix, INIT_SCALE_BALL);


    float x_distance_right = fabs((g_ball_position.x + INIT_POS_BALL.x) - (g_right_position.x + INIT_POS_RIGHT.x)) - MINIMUM_COLLISION_DISTANCE;
    float x_distance_left = fabs((g_ball_position.x + INIT_POS_BALL.x) -  (g_left_position.x + INIT_POS_LEFT.x)) - MINIMUM_COLLISION_DISTANCE;

    float y_distance_right = fabs((g_ball_position.y + INIT_POS_BALL.y) - (g_right_position.y + INIT_POS_RIGHT.y)) - MINIMUM_COLLISION_DISTANCE;
    float y_distance_left = fabs((g_ball_position.y + INIT_POS_BALL.y) - (g_left_position.y + INIT_POS_LEFT.y)) - MINIMUM_COLLISION_DISTANCE;
    
    
    if (x_distance_right <= 0.0f && y_distance_right <= 0.0f && g_ball_velocity.x > 0.0f) {
        g_ball_velocity.x = -g_ball_velocity.x;

        g_ball_position.x += g_ball_velocity.x * 0.1f;
        g_ball_position.y += g_ball_velocity.y * 0.1f;
        std::cout << std::time(nullptr) << ": Right Collision.\n";
    }

    if (x_distance_left <= 0.0f  && y_distance_left <= 0.0f && g_ball_velocity.x < 0.0f) {
        g_ball_velocity.x = -g_ball_velocity.x;

        g_ball_position.x -= g_ball_velocity.x * 0.1f;
        g_ball_position.y -= g_ball_velocity.y * 0.1f;
        std::cout << std::time(nullptr) << ": Left Collision.\n";
    }
    
    if ((g_ball_position.y + INIT_POS_BALL.y + INIT_SCALE_BALL.y / 2.0f > 3.75f)||(g_ball_position.y + INIT_POS_BALL.y - INIT_SCALE_BALL.y / 2.0f < -3.75f))  {
        g_ball_velocity.y = -g_ball_velocity.y;
        std::cout << std::time(nullptr) << ": Wall Collision.\n";
    }


    if ((g_ball_position.x + INIT_POS_BALL.x + INIT_SCALE_BALL.x / 2.0f) <= -5.0f || ((g_ball_position.x + INIT_POS_BALL.x - INIT_SCALE_BALL.x / 2.0f) >= 5.0f)) g_app_status = TERMINATED;
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_ball_matrix, g_ball_texture_id);
    draw_object(g_left_matrix, g_left_texture_id);
    draw_object(g_right_matrix, g_right_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}



