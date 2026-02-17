#include <stdio.h>
#include "alg.h"

// Funcție utilitară pentru a vedea matricea în consolă
void print_mat4(const char* name, mat4_t m) {
    printf("--- %s ---\n", name);
    for (int i = 0; i < 4; i++) {
        printf("[ %7.2f %7.2f %7.2f %7.2f ]\n", 
                m.m[i][0], m.m[i][1], m.m[i][2], m.m[i][3]);
    }
    printf("\n");
}

int main() {
    // 1. Testăm Identitatea
    mat4_t I = mat4_identity();
    print_mat4("Matricea Identitate", I);

    // 2. Testăm Rotația pe Z (90 de grade)
    // 90 grade = PI / 2
    double unghi = M_PIdiv2; 
    mat4_t rot = rot_z(&I, unghi);
    print_mat4("Rotatie Z (90 grade)", rot);

    /* Așteptări pentru Rotatie Z la 90 grade:
       cos(90) = 0, sin(90) = 1
       [  0  -1   0   0 ]
       [  1   0   0   0 ]
       [  0   0   1   0 ]
       [  0   0   0   1 ]
    */

    // 3. Testăm înmulțirea (Rotim o matrice deja rotită)
    // Daca rotim inca o data cu 90 grade, ar trebui sa ajungem la 180 (cos=-1)
    mat4_t rot180 = rot_z(&rot, unghi);
    print_mat4("Rotatie Z (180 grade total)", rot180);

    // 4. Testăm Trigonometria manuală
    printf("--- Test Trigonometrie ---\n");
    printf("sin(PI/2) = %f (asteptat: 1.0)\n", mat_sin(M_PIdiv2));
    printf("cos(PI/2) = %f (asteptat: 0.0)\n", mat_cos(M_PIdiv2));
    printf("sin(PI)   = %f (asteptat: 0.0)\n", mat_sin(M_PI));

    return 0;
}
