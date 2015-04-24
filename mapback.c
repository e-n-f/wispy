#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <jpeglib.h>

struct intersect {
	double x;
	double y;
	double mua;
	double mub;
};

struct intersect intersect(double x1, double y1, double x2, double y2,
	double x3, double y3, double x4, double y4) {

	double denom = (y4-y3) * (x2-x1) - (x4-x3) * (y2-y1);
	double numera = (x4-x3) * (y1-y3) - (y4-y3) * (x1-x3);
	double numerb = (x2-x1) * (y1-y3) - (y2-y1) * (x1-x3);

	/* Are the line coincident? */
	if (numera == 0 && numerb == 0 && denom == 0) {
		double x = (x1 + x2) / 2;
		double y = (y1 + y2) / 2;
		struct intersect ret = { x, y, -1, -1 };
		return ret;
	}

	/* Are the line parallel */
	if (denom == 0) {
		struct intersect ret = { -1, -1, -1, -1 };
		return ret;
	}

	/* Is the intersection along the the segments */
	double mua = numera / denom;
	double mub = numerb / denom;
	if (mua < 0 || mua > 1 || mub < 0 || mub > 1) {
		struct intersect ret = { -1, -1, -1, -1 };
		return ret;
	}

	double x = x1 + mua * (x2 - x1);
	double y = y1 + mua * (y2 - y1);
	struct intersect ret = { x, y, mua, mub };
	return ret;
}

void drawLine(int x0, int y0, int x1, int y1, int *image, int width, int height, int *imagemax) {
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;

	int first = 1;

	for(;;){
		if (!first) {
			if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
				int m = image[y0 * width + x0]++;
				if (m > *imagemax) {
					*imagemax = m;
				}
			}
		}
		first = 0;

		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void output(int *image, int iwidth, int iheight, int imagemax) {
	FILE *out = fopen("tmp-out.jpg", "wb");
	unsigned char buf[iwidth * 3];

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, out);

	cinfo.image_width = iwidth;
	cinfo.image_height = iheight;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 75, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	int x, y;
	for (y = iheight - 1; y >= 0; y--) {
		row_pointer[0] = buf;
		for (x = 0; x < iwidth; x++) {
#if 0
			double val = sqrt(image[y * iwidth + x]);
			val /= sqrt(imagemax);
#else
			double val = log(image[y * iwidth + x] + 1);
			val /= log(imagemax + 1);
#endif

			val *= 255;

			buf[x * 3 + 0] = val;
			buf[x * 3 + 1] = val;
			buf[x * 3 + 2] = val;
		}

		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(out);
	jpeg_destroy_compress(&cinfo);
	rename("tmp-out.jpg", "out.jpg");
}

int main(int argc, char **argv) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s width height cartogram.out\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);

	int width1 = width + 1;
	int height1 = height + 1;

	double **xpt, **ypt;
	xpt = malloc(width1 * sizeof(double *));
	ypt = malloc(width1 * sizeof(double *));
	{
		int i;
		for (i = 0; i < width1; i++) {
			xpt[i] = malloc(height1 * sizeof(double));
			ypt[i] = malloc(height1 * sizeof(double));
		}
	}

	int iwidth = width1 * 5;
	int iheight = height1 * 5;

	int *image = malloc(iwidth * iheight * sizeof(int));
	memset(image, 0, iwidth * iheight * sizeof(int));
	int imagemax = 0;

	time_t lastdump = time(NULL);

	FILE *f = fopen(argv[3], "r");
	if (f == NULL) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[3], strerror(errno));
		exit(EXIT_FAILURE);
	}

	int x, y;
	for (y = 0; y < height1; y++) {
		for (x = 0; x < width1; x++) {
			char s[2000];

			if (fgets(s, 2000, f) == NULL) {
				fprintf(stderr, "%s: %s: Early end of file at %d %d\n",
					argv[0], argv[3], x, y);
				exit(EXIT_FAILURE);
			}

			if (sscanf(s, "%lf %lf", &xpt[x][y], &ypt[x][y]) != 2) {
				fprintf(stderr, "%s: %s: Not a number pair: %s",
					argv[0], argv[2], s);
				exit(EXIT_FAILURE);
			}
		}
	}

	fclose(f);

	/* top, left, bottom, right */
	int xoff1[] = { 0, 0, 0, 1 };
	int yoff1[] = { 0, 0, 1, 0 };
	int xoff2[] = { 1, 0, 1, 1 };
	int yoff2[] = { 0, 1, 1, 1 };

	int xoffd[] = { 0, -1, 0, 1 };
	int yoffd[] = { -1, 0, 1, 0 };

	double maxdist = -1;

	char s[2000];
	while (fgets(s, 2000, stdin)) {
		double x1, y1, rx1, ry1, dist;
		double x2, y2, rx2, ry2;
		int skip = 0;

		if (sscanf(s, "%lf %lf %lf %lf %lf", &x1, &y1, &rx1, &ry1, &dist) != 5) {
			fprintf(stderr, "%s: Don't know what to do with %s", argv[0], s);
			skip = 1;
		}

		char *ignore = fgets(s, 2000, stdin);

		if (sscanf(s, "%lf %lf %lf %lf", &x2, &y2, &rx2, &ry2) != 4) {
			fprintf(stderr, "%s: Don't know what to do with %s", argv[0], s);
			skip = 1;
		}

		ignore = fgets(s, 2000, stdin);
		if (strcmp(s, "end\n") != 0) {
			fprintf(stderr, "%s: Expected \"end\", not %s", argv[0], s);
			skip = 1;
		}

		if (maxdist < 0) {
			maxdist = dist;
		}

		//printf("PS %.5f setgray\n", dist / maxdist * 254/255);
		//printf("%f %f\n", rx1, ry1);

		double linex = rx1;
		double liney = ry1;

		int boxx = rx1;
		int boxy = ry1;
		double mu = 0;
		int ok = 1;

		while (1) {
			if (boxx == (int) rx2 && boxy == (int) ry2) {
				break;
			}

			if (boxx < 0 || boxy < 0 || boxx >= width || boxy >= height) {
				fprintf(stderr, "bogus location %d %d\n", boxx, boxy);
				ok = 0;
				break;
			}

			int a;
			for (a = 0; a < width / 10; a++) {
				int xs[(a + 1) * 8];
				int ys[(a + 1) * 8];
				int n = 0;

				int xo, yo;
				for (yo = -a; yo <= a; yo++) {
					xs[n] = boxx;
					ys[n] = boxy + yo;
					n++;
				}
				for (xo = -a; xo <= a; xo++) {
					xs[n] = boxx + xo;
					ys[n] = boxy;
					n++;
				}

				int i, j;
				for (j = 0; j < n; j++) {
					for (i = 0; i < 4; i++) {
						if (xs[j] + xoff1[i] < 0) { continue; }
						if (xs[j] + xoff1[i] > width) { continue; }
						if (xs[j] + xoff2[i] < 0) { continue; }
						if (xs[j] + xoff2[i] > width) { continue; }

						if (ys[j] + yoff1[i] < 0) { continue; }
						if (ys[j] + yoff1[i] > height) { continue; }
						if (ys[j] + yoff2[i] < 0) { continue; }
						if (ys[j] + yoff2[i] > height) { continue; }

						struct intersect isect = intersect(x1, y1, x2, y2,
							xpt[xs[j] + xoff1[i]][ys[j] + yoff1[i]],
							ypt[xs[j] + xoff1[i]][ys[j] + yoff1[i]],
							xpt[xs[j] + xoff2[i]][ys[j] + yoff2[i]],
							ypt[xs[j] + xoff2[i]][ys[j] + yoff2[i]]);

						double mua = isect.mua;
						double mub = isect.mub;

						if (mua > mu) {
							double xx = xs[j] + xoff1[i] + mub *
								(xs[j] + xoff2[i] - (xs[j] + xoff1[i]));
							double yy = ys[j] + yoff1[i] + mub *
								(ys[j] + yoff2[i] - (ys[j] + yoff1[i]));

							//printf("%.6f %.6f\n", xx, yy);

							drawLine(linex * 5, liney * 5, xx * 5, yy * 5, image, iwidth, iheight, &imagemax);
							linex = xx;
							liney = yy;

							boxx = xs[j] + xoffd[i];
							boxy = ys[j] + yoffd[i];
							mu = mua;

							goto nextbox;
						}
					}
				}
			}

			if (fabs(boxx - rx2) > 2 || fabs(boxy - ry2) > 2) {
			//if (boxx != (int) rx2 || boxy != (int) ry2) {
				fprintf(stderr, "fail %f,%f to %f,%f (%f,%f to %f,%f at %d %d\n",
					x1, y1, x2, y2, rx1, ry1, rx2, ry2, boxx, boxy);

				if (boxx != (int) rx1 || boxy != (int) ry1) {
					rx1 = boxx + .5;
					ry1 = boxy + .5;
					x1 = (xpt[boxx][boxy] + xpt[boxx + 1][boxy] +
						xpt[boxx][boxy + 1] + xpt[boxx + 1][boxy + 1]) / 4.0;
					y1 = (ypt[boxx][boxy] + ypt[boxx + 1][boxy] +
						ypt[boxx][boxy + 1] + ypt[boxx + 1][boxy + 1]) / 4.0;

					fprintf(stderr, "trying again from %f,%f %f,%f\n",
						rx1, ry1, x1, y1);
					goto nextbox;
				}

				ok = 0;
			}
			break;

			nextbox: ;
		}

		if (ok) {
			//printf("%f %f\n", rx2, ry2);
			drawLine(linex * 5, liney * 5, rx2 * 5, ry2 * 5, image, iwidth, iheight, &imagemax);
		}
		//printf("end\n");

		if (time(NULL) > lastdump + 30) {
			output(image, iwidth, iheight, imagemax);
			lastdump = time(NULL);
		}
	}

	output(image, iwidth, iheight, imagemax);
	return 0;
}
