#ifndef SOLVER_H
#define SOLVER_H

// Renvoie 1 si trouvé, 0 sinon.
// Remplis res avec [y_start, x_start, y_end, x_end]
int solver(const char *grid_file, const char *word, int res[4]);

#endif