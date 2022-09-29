#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <hb.h>
#include <hb-ft.h>

#include <map>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// helper functions
	void draw_text(std::string text, float x, float y, glm::uvec2 const &drawable_size);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	FT_Library ft_library;
	FT_Face ft_face;
	FT_Error ft_error;

	hb_font_t *hb_font;

	GLuint VAO, VBO;
	GLuint vertex_shader, fragment_shader;
	GLuint program;
	GLuint texture;
	GLuint sampler;

	const char* vShaderCode = ""
        "#version 330\n"
        "in vec4 position;\n"
        "out vec2 texCoords;\n"
        "void main(void) {\n"
        "    gl_Position = vec4(position.xy, 0, 1);\n"
        "    texCoords = position.zw;\n"
        "}\n";
	
	const char *fShaderCode = ""
        "#version 330\n"
        "uniform sampler2D tex;\n"
        "in vec2 texCoords;\n"
        "out vec4 fragColor;\n"
		"const vec4 color = vec4(1, 1, 1, 1);\n"
        "void main(void) {\n"
        "    fragColor = vec4(1, 1, 1, texture(tex, texCoords).r) * color;\n"
        "}\n";
	
};
