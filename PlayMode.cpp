#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <random>

std::vector<std::string> PlayMode::split(std::string s, std::string delim) {
	auto start = 0U;
    auto end = s.find(delim);
	std::vector<std::string> res;
    while (end != std::string::npos)
    {
		res.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
    }
	res.push_back(s.substr(start));

	return res;
}

void PlayMode::draw_texts(std::string text, float x, float y, glm::uvec2 const &drawable_size) {
	auto lines = split(text, "\\n");
	for (int i = 0; i < lines.size(); i++) {
		draw_text(lines[i], x, y - 0.1f*i, drawable_size);
	}
}

void PlayMode::draw_text(std::string text, float x, float y, glm::uvec2 const &drawable_size) {
	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);

	/* Shape it! */
	hb_shape(hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);


	/* Draw the codepoints on screen. */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
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

		float xpos = x + pos[i].x_offset / (64.f * drawable_size.x);
		float ypos = y + pos[i].y_offset / (64.f * drawable_size.y);

		const float w = ft_face->glyph->bitmap.width / (float)drawable_size.x;
		const float h = ft_face->glyph->bitmap.rows / (float)drawable_size.y;

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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*6, vertices, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += pos[i].x_advance / (64.f * drawable_size.x);
		y += pos[i].y_advance / (64.f * drawable_size.y);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	GL_ERRORS();
}

void PlayMode::load_data() {
	states.clear();
	states.push_back(State());
	std::string data = data_path("data.tsv");
	std::ifstream infile(data);
	for (std::string line; std::getline(infile, line);) {
		auto vals = split(line, "\t");
		State s;
		s.text = vals[0];
		s.a = std::stoi(vals[1]);
		s.d = std::stoi(vals[2]);
		s.w = std::stoi(vals[3]);
		s.s = std::stoi(vals[4]);
		states.push_back(s);
	}
}

// snippets from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// and https://learnopengl.com/In-Practice/Text-Rendering
// and https://learnopengl.com/Getting-started/Shaders
PlayMode::PlayMode() {

	/* Generate and bind */
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(0);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glGenSamplers(1, &sampler);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindSampler(0, sampler);

	/* Initialize Shader */
	// vertex Shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vShaderCode, 0);
	glCompileShader(vertex_shader);

	// fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fShaderCode, 0);
	glCompileShader(fragment_shader);

	// attach shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);

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

	GL_ERRORS();

	load_data();
	current_state = states[1];
}

PlayMode::~PlayMode() {
	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft_library);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (left.pressed) {
		int next_index = current_state.a;
		current_state = states[next_index];
	}
	if (right.pressed) {
		int next_index = current_state.d;
		current_state = states[next_index];
	}
	if (up.pressed) {
		int next_index = current_state.w;
		current_state = states[next_index];
	}
	if (down.pressed) {
		int next_index = current_state.s;
		current_state = states[next_index];
	}

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

	draw_texts(current_state.text, -0.8f, 0.8f, drawable_size);

	GL_ERRORS();
}
