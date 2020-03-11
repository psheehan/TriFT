#include "trift.h"
#include <delaunator.hpp>
#include "timer.c"

void trift(double *x, double *y, double *flux, double *u, double *v, \
        std::complex<double> *vis, int nx, int nu) {

    // Set up the coordinates for the triangulation.

    std::vector<double> coords;

    for (int i=0; i < nx; i++) {
        coords.push_back(x[i]);
        coords.push_back(y[i]);
    }

    // Run the Delauney triangulation here.

    delaunator::Delaunator d(coords);

    // Pre-calculate the exp(-2*pi*i*rn.dot(uv)) factor as each vertex is part
    // of a number of triangles.

    double *rn_dot_uv = new double[nx*nu];
    double *sin_rn_dot_uv = new double[nx*nu];
    double *cos_rn_dot_uv = new double[nx*nu];

    TCREATE(moo); TCLEAR(moo); TSTART(moo);
    for (int i = 0; i < nx; i++) {
        Vector <double, 3> rn(x[i], y[i], 0.);

        std::size_t idx = i * nu;

        for (std::size_t j = 0; j < (std::size_t) nu; j++) {
            Vector <double, 3> uv(u[j], v[j], 0.);

            rn_dot_uv[idx + j] = rn.dot(uv);
            cos_rn_dot_uv[idx + j] = cos(2.*pi*rn_dot_uv[idx + j]);
            sin_rn_dot_uv[idx + j] = sin(2.*pi*rn_dot_uv[idx + j]);
        }
    }
    TSTOP(moo);
    printf("%f\n", TGIVE(moo));

    // Loop through and take the Fourier transform of each triangle.
    
    Vector<double, 3> zhat(0., 0., 1.);

    double *vis_real = new double[nu];
    double *vis_imag = new double[nu];

    for (std::size_t i = 0; i < d.triangles.size(); i+=3) {
        double intensity_triangle = (flux[d.triangles[i]] + 
            flux[d.triangles[i+1]] + flux[d.triangles[i+2]]) / 3.;

        for (int j = 0; j < 3; j++) {
            // Calculate the vectors for the vertices of the triangle.

            std::size_t i_rn1 = d.triangles[i + (j+1)%3];
            Vector<double, 3> rn1(x[i_rn1], y[i_rn1],  0.);

            std::size_t i_rn = d.triangles[i + j];
            Vector<double, 3> rn(x[i_rn], y[i_rn],  0.);

            std::size_t i_rn_1 = d.triangles[i + (j+2)%3];
            Vector<double, 3> rn_1(x[i_rn_1], y[i_rn_1],  0.);

            // Calculate the vectors for the edges of the triangle.

            Vector<double, 3> ln = rn1 - rn;
            Vector<double, 3> ln_1 = rn - rn_1;

            // Now loop through the UV points and calculate the Fourier
            // Transform.

            double ln_1_dot_zhat_cross_ln = ln_1.dot(zhat.cross(ln));

            std::size_t idx = i_rn * nu;

            for (std::size_t k = 0; k < (std::size_t) nu; k++) {
                Vector <double, 3> uv(u[k], v[k], 0.);
                
                vis_real[k] += intensity_triangle * ln_1_dot_zhat_cross_ln /
                    (ln.dot(uv) * ln_1.dot(uv)) * cos_rn_dot_uv[idx + k];
                vis_imag[k] += intensity_triangle * ln_1_dot_zhat_cross_ln /
                    (ln.dot(uv) * ln_1.dot(uv)) * sin_rn_dot_uv[idx + k];
            }
        }
    }

    // Now add real and imaginary together.

    std::complex<double> I(0,1);

    for (std::size_t i = 0; i < (std::size_t) nu; i++) {
        vis[i] = vis_real[i] + I*vis_imag[i];
    }

    // Clean up.

    delete[] rn_dot_uv; delete[] sin_rn_dot_uv; delete[] cos_rn_dot_uv;
    delete[] vis_real; delete[] vis_imag;
}
