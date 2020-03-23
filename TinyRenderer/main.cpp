#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor WHITE = TGAColor(255, 255, 255, 255);
const TGAColor RED = TGAColor(255, 0, 0, 255);
const TGAColor GREEN = TGAColor(0, 255, 0, 255);
const int WIDTH = 800;
const int HEIGHT = 800;
const int DEPTH = 255;


Model *model = NULL;
int *zbuffer = NULL;
Vec3f light_dir(0, 0, -1);

void line(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color);
void triangle(Vec3i t0, Vec3i t1, Vec3i t2, TGAImage &image, TGAColor color, int *zbuffer);
void rasterize(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color, int ybuffer[]);

Vec3f world2screen(Vec3f v) {
	return Vec3f(int((v.x + 1.)*WIDTH / 2. + .5), int((v.y + 1.)*HEIGHT / 2. + .5), v.z);
}

int main(int argc, char** argv) {
	if (argc == 2) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/head.obj");
	}

	TGAImage image(HEIGHT, WIDTH, TGAImage::RGB);

//  WIREFRAME
//	for (int i = 0; i < model->nfaces(); i++) {
//		std::vector<int> face = model->face(i);
//		for (int j = 0; j < 3; j++) {
//			Vec3f v0 = model->vert(face[j]);
//			Vec3f v1 = model->vert(face[(j + 1) % 3]);
//			int x0 = (v0.x + 1.)*WIDTH / 2.;
//			int y0 = (v0.y + 1.)*HEIGHT / 2.;
//			int x1 = (v1.x + 1.)*WIDTH / 2.;
//			int y1 = (v1.y + 1.)*HEIGHT / 2.;
//
//			Vec2i p0(x0, y0);
//			Vec2i p1(x1, y1);
//			line(p0, p1, image, WHITE);
//
//		}
//
//	}

	zbuffer = new int[WIDTH*HEIGHT];
	for (int i = 0; i < WIDTH*HEIGHT; i++) {
		zbuffer[i] = std::numeric_limits<int>::min();
	}

	{ // draw the model
		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			Vec3i screen_coords[3];
			Vec3f world_coords[3];
			for (int j = 0; j < 3; j++) {
				Vec3f v = model->vert(face[j]);
				screen_coords[j] = Vec3i((v.x + 1.)*WIDTH / 2., (v.y + 1.)*HEIGHT / 2., (v.z + 1.)*DEPTH / 2.);
				world_coords[j] = v;
			}
			Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
			n.normalize();
			float intensity = n * light_dir;
			if (intensity > 0) {
				triangle(screen_coords[0], screen_coords[1], screen_coords[2], image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255), zbuffer);
			}
		}

		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("output.tga");
	}
	delete model;
	return 0;
}

void rasterize(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color, int ybuffer[]) {
	if (p0.x > p1.x) {
		std::swap(p0, p1);
	}
	for (int x = p0.x; x <= p1.x; x++) {
		float t = (x - p0.x) / (float)(p1.x - p0.x);
		int y = p0.y*(1. - t) + p1.y*t;
		if (ybuffer[x] < y) {
			ybuffer[x] = y;
			image.set(x, 0, color);
		}
	}
}

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, TGAImage &image, TGAColor color, int *zbuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
		Vec3i A = t0 + (t2 - t0)*alpha;
		Vec3i B = second_half ? t1 + (t2 - t1)*beta : t0 + (t1 - t0)*beta;
		if (A.x > B.x) std::swap(A, B);
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3i P = A + (B - A)*phi;
			P.x = j; P.y = t0.y + i; // a hack to fill holes (due to int cast precision problems)
			int idx = j + (t0.y + i)*WIDTH;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				image.set(P.x, P.y, color); // attention, due to int casts t0.y+i != A.y
			}
		}
	}
}

void line(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color) {
	bool steep = false;
	if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
		std::swap(p0.x, p0.y);
		std::swap(p1.x, p1.y);
		steep = true;
	}
	if (p0.x > p1.x) {
		std::swap(p0, p1);
	}

	for (int x = p0.x; x <= p1.x; x++) {
		float t = (x - p0.x) / (float)(p1.x - p0.x);
		int y = p0.y*(1. - t) + p1.y*t;
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
	}
}