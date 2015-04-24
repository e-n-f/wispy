#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

struct point {
	int lat;
	int lon;
	struct point *next;
};

int platcmp(const void *v1, const void *v2) {
	const struct point *p1 = v1;
	const struct point *p2 = v2;

	int l1 = abs(p1->lat);
	int l2 = abs(p2->lat);

	return l1 - l2;
}

int width, height;
double scale;

int getdist(int lat, int lon, struct point **grid, int x, int y, int d) {
	if (x < 0 || y < 0 || x >= width || y >= height) {
		return d;
	}

	//printf("at %d %d\n", x, y);

	struct point *p = grid[y * width + x];
	for (; p != NULL; p = p->next) {
		double latd = lat - p->lat;
		double lond = (lon - p->lon) * scale;

		double d2 = sqrt(latd * latd + lond * lond);

		//printf("   try %d %d to %d %d  dist %f\n", lat, lon, p->lat, p->lon, d2);

		if (d2 < d) {
			d = d2;
		}
	}

	return d;
}

int count(struct point *grid) {
	int n = 0;
	while (grid != NULL) {
		n++;
		grid = grid->next;
	}
	return n;
}

int main() {
	char s[2000];
	char user[2000], date[2000], time[2000];
	char odate[2000];

	struct point *points = NULL;
	int n = 0;
	int max = 0;

	int minlat = INT_MAX, minlon = INT_MAX;
	int maxlat = INT_MIN, maxlon = INT_MIN;

	while (fgets(s, 2000, stdin)) {
		float lat, lon;
		if (sscanf(s, "%s %s %s %f,%f", user, date, time, &lat, &lon) == 5) {
			//printf("%f %f\n", lat, lon);

			if (strcmp(date, odate) != 0) {
				//fprintf(stderr, "%s\n", date);
				strcpy(odate, date);
			}

			if (n >= max) {
				fprintf(stderr, "%d\n", n);
				max = n + 100000;

				points = realloc(points, max * sizeof(struct point));
				if (points == NULL) {
					fprintf(stderr, "fail for %d\n", max);
					exit(1);
				}
			}

			points[n].lat = lat * 1000000;
			points[n].lon = lon * 1000000;

			if (points[n].lat > maxlat) {
				maxlat = points[n].lat;
			}
			if (points[n].lat < minlat) {
				minlat = points[n].lat;
			}
			if (points[n].lon > maxlon) {
				maxlon = points[n].lon;
			}
			if (points[n].lon < minlon) {
				minlon = points[n].lon;
			}

			n++;
		}
	}

        qsort(points, n, sizeof(struct point), platcmp);
        int midlat = points[n / 2].lat;

	scale = cos(midlat * M_PI / 180000000);

	width = 1000;
	height = width / scale / (maxlon - minlon) * (maxlat - minlat);

	//printf("%d by %d scale %f\n", width, height, scale);
	//printf("%d %d to %d %d: %d\n", minlat, minlon, maxlat, maxlon, midlat);

	struct point **grid = malloc(width * height * sizeof(struct point *));
	int *out = malloc(width * height * sizeof(int));
	if (grid == NULL || out == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memset(grid, 0, width * height * sizeof(struct point *));

	fprintf(stderr, "%d %d\n", width, height);
	fprintf(stderr, "%.6f %.6f %.6f %.6f\n", minlat / 1000000.0 , minlon / 1000000.0 , maxlat / 1000000.0 , maxlon / 1000000.0);

	int i;
	for (i = 0; i < n; i++) {
		int x = ((long long) points[i].lon - minlon) * width / (maxlon - minlon);
		int y = ((long long) points[i].lat - minlat) * height / (maxlat - minlat);

		if (x < 0 || y < 0 || x >= width || y >= height) {
			fprintf(stderr, "shouldn't happen: %d %d\n", x, y);
			continue;
		}

		points[i].next = grid[y * width + x];
		grid[y * width + x] = &points[i];
	}

	int x, y;
	int maxd = 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			//printf("FOR %d %d\n", x, y);

			int lon = (x + .5) * (maxlon - minlon) / width + minlon;
			int lat = (y + .5) * (maxlat - minlat) / height + minlat;

			int d = getdist(lat, lon, grid, x, y, INT_MAX);

			if (1 || d == INT_MAX) {
				int a;

				for (a = 1; a < width; a++) {
					int xx, yy;

					//printf("loop %d\n", a);

					for (xx = x - a; xx <= x + a; xx++) {
						d = getdist(lat, lon, grid, xx, y - a, d);
						d = getdist(lat, lon, grid, xx, y + a, d);
					}

					for (yy = y - a + 1; yy <= y + a - 1; yy++) {
						d = getdist(lat, lon, grid, x - a, yy, d);
						d = getdist(lat, lon, grid, x + a, yy, d);
					}

					if (d != INT_MAX &&
						 d < (double) a * (maxlon - minlon) / width) {
						break;
					}
				}
			} else {
				d = ((double) 1 * (maxlon - minlon) / width) / (
					count(grid[y * width + x]) * count(grid[y * width + x]));
				//printf("really?\n");
			}

			out[y * width + x] = d;
			if (d > maxd) {
				maxd = d;
			}
		}

		//fprintf(stderr,"%d\n", y);
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			printf("%.6f ", 255.0 * out[y * width + x] / maxd);
		}
		printf("\n");
	}

	fprintf(stderr, "%d %d\n", width, height);
	fprintf(stderr, "%.6f %.6f %.6f %.6f\n", minlat / 1000000.0 , minlon / 1000000.0 , maxlat / 1000000.0 , maxlon / 1000000.0);

	return 0;
}
