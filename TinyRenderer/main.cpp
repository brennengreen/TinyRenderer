#include <cmath>
#include <vector>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor WHITE = TGAColor(255, 255, 255, 255);
const TGAColor RED = TGAColor(255, 0, 0, 255);
const int WIDTH = 800;
const int HEIGHT = 800;

Model *model = NULL;

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color);

int main(int argc, char** argv) {
	if (argc == 2) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/head.obj");
	}

	TGAImage image(HEIGHT, WIDTH, TGAImage::RGB);
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++) {
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (v0.x + 1.)*WIDTH / 2.;
			int y0 = (v0.y + 1.)*HEIGHT / 2.;
			int x1 = (v1.x + 1.)*WIDTH / 2.;
			int y1 = (v1.y + 1.)*HEIGHT / 2.;
			line(x0, y0, x1, y1, image, WHITE);

		}

	}

	image.flip_vertically(); // Set the origin at the bottom left corner
	image.write_tga_file("output.tga");
	delete model;
	return 0;
}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) { // If line is steep, transpose the image
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}	

	if (x0 > x1) { // Ensure rendering from left-to-right
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	float derror2 = std::abs(dy) * 2;
	float error2 = 0;
	int y = y0;

	for (int x = x0; x <= x1; x++) {
		if (steep) { // Detranspose
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}