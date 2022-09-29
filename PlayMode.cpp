#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

void PlayMode::draw_text(std::string text, float x, float y, glm::uvec2 const &drawable_size) {
	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);

	// /* Shape it! */
	hb_shape(hb_font, hb_buffer, NULL, 0);

	// /* Get glyph information and positions out of the buffer. */
	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);


	/* Draw the codepoints on screen. */
	for (unsigned int i = 0; i < len; i++) {
		hb_codepoint_t glyph_index = info[i].codepoint;

		ft_error = FT_Load_Glyph( ft_face, glyph_index, FT_LOAD_DEFAULT );
		if ( ft_error ) {
			std::cout << "Failed to load glyph.\n";
    		continue;
		}
		ft_error = FT_Render_Glyph( ft_face->glyph, FT_RENDER_MODE_NORMAL );
		if ( ft_error ) {
			std::cout << "Failed to render glyph.\n";
    		continue;
		}

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			ft_face->glyph->bitmap.width,
			ft_face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			ft_face->glyph->bitmap.buffer
		);


		float xpos = x + pos[i].x_offset;
		float ypos = y + pos[i].x_offset;

		float w = ft_face->glyph->bitmap.width / drawable_size.x;
		float h = ft_face->glyph->bitmap.rows / drawable_size.y;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },            
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }           
		};

		// bind buffer and texture
		glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), vertices, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += pos[i].x_advance;
		y += pos[i].y_advance;
	}

	GL_ERRORS();
}

// snippets from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// and https://learnopengl.com/In-Practice/Text-Rendering
// and https://learnopengl.com/Getting-started/Shaders
PlayMode::PlayMode() {

	/* Create VAO and VBO */
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	/* Initialize Shader */
	// vertex Shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vShaderCode, NULL);

	// fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fShaderCode, NULL);
	glCompileShader(fragment_shader);

	// attach shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	
	// delete the shaders as they're linked into our program now and no longer necessary
	// glDeleteShader(vertex_shader);
	// glDeleteShader(fragment_shader);


	/* Initialize some GL states*/
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 1);
	
	// https://fonts.google.com/specimen/Roboto
	std::string regular_font = data_path("./Roboto/Roboto-Regular.ttf");

	constexpr int FONT_SIZE = 100;
	/* Initialize FreeType and create FreeType font face. */
	if ((ft_error = FT_Init_FreeType (&ft_library)))
		abort();
	if ((ft_error = FT_New_Face (ft_library, regular_font.c_str(), 0, &ft_face)))
		abort();
	if ((ft_error = FT_Set_Char_Size (ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0)))
		abort();

	/* Create hb-ft font. */
	hb_font = hb_ft_font_create (ft_face, NULL);

	// load something at the start of game
	
	GL_ERRORS();
}

PlayMode::~PlayMode() {
	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft_library);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = true;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//reset button press:
	left.pressed = false;
	right.pressed = false;
	up.pressed = false;
	down.pressed = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	// glBindSampler(0, sampler);
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glUseProgram(program);

	draw_text("A", -0.8f, 0.8f, drawable_size);

	GL_ERRORS();
}
