#include "CpuANISymmetryFunctions.h"
#include <cmath>
#include <stdexcept>
#include <string>

using namespace std;

void assertEqual(float v1, float v2, float atol, float rtol) {
    float diff = fabs(v1-v2);
    if (diff > atol && diff/v1 > rtol)
        throw runtime_error(string("Assertion failure: expected ")+to_string(v1)+" found "+to_string(v2));
}

void validateDerivatives(ANISymmetryFunctions& ani, float* positions, float* periodicVectors) {
    int numAtoms = ani.getNumAtoms();
    int numSpecies = ani.getNumSpecies();
    int numRadial = numAtoms*numSpecies*ani.getRadialFunctions().size();
    int numAngular = numAtoms*numSpecies*(numSpecies+1)*ani.getAngularFunctions().size()/2;
    vector<float> allValues(numRadial+numAngular);
    float* radial = &allValues[0];
    float* angular = &allValues[numRadial];
    vector<float> allDeriv(numRadial+numAngular, 0);
    float* radialDeriv = &allDeriv[0];
    float* angularDeriv = &allDeriv[numRadial];
    vector<float> positionDeriv(numAtoms*3);
    vector<float> offsetPositions(numAtoms*3);
    float step = 1e-3;
    for (int i = 0; i < numRadial+numAngular; i++) {
        // Use backprop to compute the gradient of one symmetry function.

        ani.computeSymmetryFunctions(positions, periodicVectors, radial, angular);
        allDeriv[i] = 1;
        ani.backprop(radialDeriv, angularDeriv, positionDeriv.data());
        allDeriv[i] = 0;

        // Displace the atoms along the gradient direction, compute the symmetry functions,
        // and calculate a finite difference approximation to the gradient magnitude from them.

        float norm = 0;
        for (int j = 0; j < positionDeriv.size(); j++)
            norm += positionDeriv[j]*positionDeriv[j];
        norm = sqrt(norm);
        float delta = step/norm;
        for (int j = 0; j < offsetPositions.size(); j++)
            offsetPositions[j] = positions[j] - delta*positionDeriv[j];
        ani.computeSymmetryFunctions(offsetPositions.data(), periodicVectors, radial, angular);
        float value1 = allValues[i];
        for (int j = 0; j < offsetPositions.size(); j++)
            offsetPositions[j] = positions[j] + delta*positionDeriv[j];
        ani.computeSymmetryFunctions(offsetPositions.data(), periodicVectors, radial, angular);
        float value2 = allValues[i];
        float estimate = (value2-value1)/(2*step);

        // Verify that they match.

        assertEqual(norm, estimate, 1e-5, 5e-3);
    }
}

void testWater(bool torchani, float* periodicVectors, float* expectedRadial, float* expectedAngular) {
    int numAtoms = 18;
    int numSpecies = 2;
    float positions[18][3] = {
        { 0.726, -1.384, -0.376},
        {-0.025, -0.828, -0.611},
        { 1.456, -1.011, -0.923},
        {-1.324,  0.387, -0.826},
        {-1.923,  0.698, -1.548},
        {-1.173,  1.184, -0.295},
        { 0.837, -1.041,  2.428},
        { 1.024, -1.240,  1.461},
        { 1.410, -1.677,  2.827},
        { 2.765,  0.339, -1.505},
        { 2.834,  0.809, -0.685},
        { 3.582, -0.190, -1.593},
        {-0.916,  2.705,  0.799},
        {-0.227,  2.580,  1.426},
        {-0.874,  3.618,  0.468},
        {-2.843, -1.749,  0.001},
        {-2.928, -2.324, -0.815},
        {-2.402, -0.876, -0.235}
    };
    vector<int> species = {0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1};
    vector<RadialFunction> radialFunctions = {
        {5.0, 2.0},
        {5.0, 3.0}
    };
    vector<AngularFunction> angularFunctions = {
        {5.0, 1.0, 10.0, 0.5},
        {5.0, 1.0, 10.0, 1.5},
        {5.0, 2.0, 10.0, 0.5},
        {5.0, 2.0, 10.0, 1.5}
    };
    CpuANISymmetryFunctions ani(numAtoms, numSpecies, 4.5, 3.5, (periodicVectors != NULL), species, radialFunctions, angularFunctions, torchani);
    if (torchani) {
        vector<float> radial(numAtoms*numSpecies*radialFunctions.size());
        vector<float> angular(numAtoms*numSpecies*(numSpecies+1)*angularFunctions.size()/2);
        ani.computeSymmetryFunctions((float*) positions, periodicVectors, radial.data(), angular.data());
        for (int i = 0; i < radial.size(); i++)
            assertEqual(expectedRadial[i], radial[i], 1e-4, 1e-3);
        for (int i = 0; i < angular.size(); i++) {
            assertEqual(expectedAngular[i], angular[i], 1e-4, 1e-3);
        }
    }
    validateDerivatives(ani, (float*) positions, periodicVectors);
}

void testWaterNonperiodic() {
    // Values computed with TorchANI.
    float expectedRadial[] = {
        0.008830246, 0.1957682, 0.14725034, 0.19745183, 0.13371678, 0.14999035, 0.25027117, 0.14884546,
        0.15007327, 0.066582225, 0.23719531, 0.05880443, 0.01192417, 0.18637833, 0.26062375, 0.13020146,
        0.001616041, 0.09718165, 0.20872512, 0.07887973, 0.14815442, 0.057246655, 0.24792309, 0.10041529,
        0.0024850904, 0.065443136, 0.0024688751, 0.05918453, 0.14615968, 0.003345328, 0.15343817, 0.048463706,
        0.00086265814, 0.027797509, 0.050137855, 0.0010170789, 0.0012426941, 0.06679656, 0.15080662, 0.04700415,
        0.0010976682, 0.057259407, 0.14324662, 0.0469618, 0.0011948426, 0.023307199, 0.12130508, 0.016885059,
        0.0018155162, 0.06651578, 0.14897372, 0.03345788, 0.0008188619, 0.023132995, 0.115777194, 0.0197288,
        0.0011286231, 0.007843749, 0.08712131, 0.040015582, 0.005006309, 0.063744456, 0.003675533, 0.13625659,
        0.0015837244, 0.046651784, 0.09421141, 0.060269244, 0.12739228, 0.043217584, 0.32347643, 0.040697813
    };
    float expectedAngular[] = {
        8.91936e-11, 3.2934735e-09, 4.06611e-05, 0.0015464495, 0.0058859047, 0.00848414, 0.31403738, 0.5093584,
        0.01884812, 1.486223, 0.13770749, 0.33304307, 9.9263285e-05, 0.005042704, 0.033768307, 0.10968382,
        0.66707796, 0.20471708, 0.72144556, 0.42768338, 0.000646298, 0.004465868, 0.11549817, 0.52482647,
        8.949461e-05, 0.007458916, 0.023044452, 0.04074476, 0.6459322, 0.18171918, 0.6067002, 0.33666277,
        0.0005831143, 0.0043910975, 0.13145529, 0.46435228, 5.814406e-10, 7.263835e-09, 0.00015368589, 0.0022230423,
        0.0020642858, 0.01063498, 0.27814752, 0.65119684, 0.028670194, 1.9464642, 0.06349146, 0.7397544,
        0.00016283127, 0.00031536253, 0.033458617, 0.06116189, 0.67979735, 0.21525048, 0.37555915, 0.19569798,
        0.0017871787, 0.014071389, 0.13010885, 0.502967, 5.1683553e-05, 0.002567268, 0.01922456, 0.01868274,
        0.6832994, 0.18723322, 0.69772875, 0.28333214, 0.0014293044, 0.005412821, 0.18807307, 0.60809433,
        0.0, 0.0, 0.0, 0.0, 0.0021140398, 0.0019475736, 0.13521858, 0.09368172,
        0.024592534, 1.1837298, 0.036000174, 0.023186857, 8.290156e-08, 0.007918005, 4.358124e-08, 0.0041624857,
        0.6435361, 0.1763118, 0.42178524, 0.09398934, 5.3416516e-06, 0.00019628856, 0.04161539, 0.02677626,
        1.6963973e-05, 1.432901e-05, 0.008062218, 0.0068099382, 0.66431797, 0.2209945, 0.04345796, 0.012960012,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        8.884623e-05, 0.0026747673, 0.027179977, 0.19587424, 0.017809598, 1.3922685, 0.011441946, 0.33338425,
        9.5205716e-05, 0.0003685832, 0.014500167, 0.056136522, 0.6774628, 0.18752435, 0.15832098, 0.18087253,
        0.00089232856, 0.0041441186, 0.0593719, 0.27821755, 6.727289e-06, 7.2187026e-06, 0.004552609, 0.004885167,
        0.65077364, 0.15037178, 0.18514359, 0.12859476, 0.0007789619, 0.0022341206, 0.07704024, 0.22095713,
        0.0, 0.0, 0.0, 0.0, 1.3919507e-05, 0.002124481, 0.032363284, 0.12334709,
        0.008677481, 1.2315067, 0.002804904, 0.15479667, 9.196073e-06, 7.4327013e-06, 0.0052442392, 0.0042386428,
        0.6803334, 0.15231898, 0.20884977, 0.0899671, 0.00030674934, 0.0020051494, 0.03785962, 0.24747954,
        0.0, 0.0, 0.0, 0.0, 0.64348125, 0.12322514, 0.24395435, 0.054215096,
        0.00021477071, 0.00053840934, 0.05863409, 0.14698999, 0.0, 0.0, 0.0, 0.0,
        0.0036240458, 0.0031566615, 0.15795282, 0.13455567, 0.0066748317, 0.87194496, 0.13082358, 0.16623536,
        5.8405854e-05, 7.106351e-05, 0.018520605, 0.022534369, 0.4839859, 0.09273548, 0.10434935, 0.041937273,
        4.771605e-07, 1.3412289e-07, 0.0069035515, 0.0019494988, 8.062944e-07, 0.0063289064, 0.0033217252, 0.030469447,
        0.48621517, 0.119468495, 0.7752426, 0.3422918, 8.571804e-05, 0.0045413026, 0.14614908, 0.4854161
    };
    testWater(true, NULL, expectedRadial, expectedAngular);
    testWater(false, NULL, NULL, NULL);
}

void testWaterPeriodic() {
    // Values computed with TorchANI.
    float expectedRadial[] = {
        0.008830246, 0.1957682, 0.14725034, 0.19745186, 0.13371678, 0.14999035, 0.25027117, 0.14884546,
        0.15007327, 0.066582225, 0.23719531, 0.05880443, 0.01192417, 0.18637833, 0.26062375, 0.1302033,
        0.001616041, 0.09718178, 0.20872518, 0.082626775, 0.14815442, 0.057246655, 0.24792309, 0.10041529,
        0.0024850904, 0.065443136, 0.0024688751, 0.05918453, 0.14615968, 0.003345328, 0.15343817, 0.048463706,
        0.00086265814, 0.027797509, 0.050137855, 0.0010170789, 0.0012426941, 0.06679721, 0.15080662, 0.047006022,
        0.0010976682, 0.057260185, 0.14324662, 0.04696705, 0.001196608, 0.03852256, 0.121310964, 0.05704461,
        0.0018155162, 0.06651578, 0.14897372, 0.03345788, 0.0008188619, 0.023132995, 0.115777194, 0.0197288,
        0.0011286231, 0.007848075, 0.08712131, 0.040201984, 0.005006309, 0.0637451, 0.0036772983, 0.15147519,
        0.0015837244, 0.046652038, 0.09421449, 0.07904015, 0.12739228, 0.04321907, 0.32347918, 0.058531057
    };
    float expectedAngular[] = {
        8.91936e-11, 3.2934735e-09, 4.06611e-05, 0.0015464495, 0.0058859047, 0.008484141, 0.31403735, 0.5093584,
        0.01884812, 1.4862229, 0.13770749, 0.3330431, 9.9263285e-05, 0.005042704, 0.033768307, 0.10968382,
        0.66707796, 0.2047171, 0.7214456, 0.42768335, 0.000646298, 0.004465868, 0.115498185, 0.52482647,
        8.949461e-05, 0.007458916, 0.023044452, 0.04074476, 0.64593226, 0.1817192, 0.60669994, 0.33666277,
        0.0005831142, 0.0043910975, 0.13145527, 0.46435225, 5.814406e-10, 7.263835e-09, 0.00015368589, 0.0022230423,
        0.0020642858, 0.010634979, 0.27814752, 0.65119684, 0.028670192, 1.9464641, 0.06349146, 0.7397544,
        0.00016283127, 0.00031536253, 0.033458617, 0.06116189, 0.6797973, 0.21525046, 0.37555915, 0.19569796,
        0.0017871787, 0.014071389, 0.13010885, 0.502967, 5.1683553e-05, 0.002567268, 0.01922456, 0.01868274,
        0.6832994, 0.18723324, 0.6977289, 0.28333214, 0.0014293044, 0.0054128217, 0.18807307, 0.60809433,
        0.0, 0.0, 0.0, 0.0, 0.0021140396, 0.0019475736, 0.13521856, 0.09368173,
        0.024592536, 1.1837298, 0.03600017, 0.023186859, 8.290156e-08, 0.007918005, 4.358124e-08, 0.0041624857,
        0.64353603, 0.1763118, 0.4217852, 0.09398935, 5.341652e-06, 0.00019628854, 0.04161539, 0.02677626,
        1.6963973e-05, 1.432901e-05, 0.008062218, 0.0068099382, 0.66431797, 0.2209945, 0.04345796, 0.012960012,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        8.884624e-05, 0.0026747675, 0.027179977, 0.19587424, 0.017809596, 1.3922687, 0.011441946, 0.33338422,
        9.5205716e-05, 0.0003685832, 0.014500167, 0.056136522, 0.6774628, 0.18752435, 0.15832098, 0.18087253,
        0.00089232856, 0.0041441186, 0.059371904, 0.27821752, 6.7273013e-06, 7.403339e-06, 0.0045526214, 0.0050715576,
        0.6507736, 0.15037239, 0.18514578, 0.12950988, 0.00077896233, 0.002234204, 0.07704706, 0.22221094,
        0.0, 0.0, 0.0, 0.0, 1.3919507e-05, 0.002124481, 0.032363284, 0.12334709,
        0.008677481, 1.2315067, 0.002804904, 0.15479667, 9.196073e-06, 7.4327013e-06, 0.0052442392, 0.0042386428,
        0.6803334, 0.15231898, 0.20884977, 0.0899671, 0.00030674934, 0.0020051494, 0.03785962, 0.24747954,
        0.0, 0.0, 0.0, 0.0, 0.64348125, 0.12322514, 0.24395435, 0.054215096,
        0.00021477071, 0.00053840934, 0.05863409, 0.14698999, 0.0, 0.0, 0.0, 0.0,
        0.003624046, 0.0031566615, 0.15795292, 0.13455684, 0.0066753277, 0.87194896, 0.13139585, 0.17077583,
        5.8405854e-05, 7.106351e-05, 0.018520605, 0.022534369, 0.48398635, 0.09274029, 0.1047609, 0.0465349,
        4.840144e-07, 1.7399041e-07, 0.007067506, 0.0029031872, 8.0629434e-07, 0.0063289064, 0.0033217252, 0.030469447,
        0.48621553, 0.11947246, 0.77562207, 0.34674916, 8.572407e-05, 0.0045413356, 0.14631495, 0.48638728
    };
    float periodicVectors[] = {
        9.0, 0.0, 0.0,
        0.0, 9.0, 0.0,
        0.0, 0.0, 9.0
    };
    testWater(true, periodicVectors, expectedRadial, expectedAngular);
    testWater(false, periodicVectors, NULL, NULL);
}

void testWaterTriclinic() {
    // Values computed with TorchANI.
    float expectedRadial[] = {
        0.008830246, 0.1957682, 0.14725034, 0.19745183, 0.13371678, 0.14999035, 0.25027117, 0.14884546,
        0.15007327, 0.066582225, 0.23719531, 0.05880443, 0.01192417, 0.18637833, 0.26062375, 0.1302033,
        0.001616041, 0.09718178, 0.20872518, 0.082626775, 0.14815442, 0.057246655, 0.24792309, 0.10041529,
        0.0024850904, 0.065443136, 0.0024688751, 0.05918453, 0.14615968, 0.003345328, 0.15343817, 0.048463706,
        0.00086265814, 0.027797509, 0.050137855, 0.0010170789, 0.0012426941, 0.06679721, 0.15080662, 0.047006022,
        0.0010976682, 0.057260185, 0.14324662, 0.04696705, 0.001196608, 0.03852256, 0.121310964, 0.05704461,
        0.0018155162, 0.06651578, 0.14897372, 0.033458054, 0.0008188619, 0.023132995, 0.115777194, 0.0197288,
        0.0011286347, 0.009597496, 0.0871248, 0.059427276, 0.005006309, 0.0637451, 0.00367731, 0.15322463,
        0.0015837244, 0.046652213, 0.094217986, 0.09826544, 0.12739228, 0.04321907, 0.32347918, 0.058531057
    };
    float expectedAngular[] = {
        8.91936e-11, 3.2934735e-09, 4.06611e-05, 0.0015464495, 0.0058859047, 0.008484141, 0.31403735, 0.5093584,
        0.01884812, 1.4862229, 0.13770749, 0.3330431, 9.926328e-05, 0.0050427043, 0.03376831, 0.10968381,
        0.66707796, 0.20471707, 0.7214456, 0.42768335, 0.000646298, 0.004465868, 0.11549817, 0.52482647,
        8.9494606e-05, 0.007458916, 0.02304445, 0.040744755, 0.6459322, 0.1817192, 0.6067, 0.33666277,
        0.0005831143, 0.0043910975, 0.13145527, 0.46435222, 5.814406e-10, 7.263835e-09, 0.00015368589, 0.0022230423,
        0.0020642858, 0.01063498, 0.27814755, 0.65119684, 0.028670195, 1.9464641, 0.06349146, 0.7397543,
        0.00016283127, 0.00031536253, 0.033458617, 0.06116189, 0.6797973, 0.21525048, 0.37555915, 0.19569798,
        0.0017871788, 0.014071389, 0.13010885, 0.502967, 5.1683553e-05, 0.002567268, 0.01922456, 0.018682742,
        0.6832995, 0.18723322, 0.69772875, 0.28333214, 0.0014293044, 0.0054128217, 0.18807307, 0.6080944,
        0.0, 0.0, 0.0, 0.0, 0.0021140396, 0.0019475736, 0.13521856, 0.09368172,
        0.024592536, 1.1837299, 0.036000174, 0.023186857, 8.290156e-08, 0.007918005, 4.358124e-08, 0.0041624857,
        0.64353603, 0.1763118, 0.42178524, 0.09398934, 5.341652e-06, 0.00019628856, 0.04161539, 0.02677626,
        1.6963973e-05, 1.432901e-05, 0.008062218, 0.0068099382, 0.66431797, 0.2209945, 0.04345796, 0.012960012,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        8.884624e-05, 0.0026747673, 0.027179979, 0.19587424, 0.017809596, 1.3922687, 0.011441946, 0.33338422,
        9.5205716e-05, 0.0003685832, 0.014500167, 0.056136522, 0.6774628, 0.18752435, 0.15832098, 0.18087253,
        0.00089232856, 0.0041441186, 0.0593719, 0.27821755, 6.7273013e-06, 7.403339e-06, 0.0045526214, 0.0050715576,
        0.6507736, 0.1503724, 0.18514578, 0.12950988, 0.00077896233, 0.002234204, 0.07704706, 0.22221094,
        0.0, 0.0, 0.0, 0.0, 1.3919507e-05, 0.002124481, 0.032363284, 0.12334709,
        0.008677481, 1.2315067, 0.002804904, 0.15479669, 9.196073e-06, 7.4327013e-06, 0.0052442392, 0.0042386428,
        0.6803334, 0.15231898, 0.20884977, 0.0899671, 0.00030674934, 0.0020051494, 0.03785962, 0.24747954,
        0.0, 0.0, 0.0, 0.0, 0.64348125, 0.123225234, 0.24395435, 0.054291688,
        0.00021477071, 0.00053841335, 0.05863409, 0.14704987, 0.0, 0.0, 0.0, 0.0,
        0.0036240458, 0.0031566615, 0.1579529, 0.13455684, 0.0066753277, 0.87194896, 0.13139586, 0.17077585,
        5.8405854e-05, 7.106351e-05, 0.018520605, 0.022534369, 0.4839864, 0.09274514, 0.10483521, 0.050971314,
        4.840281e-07, 1.8900553e-07, 0.00706782, 0.0032480082, 8.0629434e-07, 0.0063289064, 0.0033217252, 0.030469447,
        0.48621553, 0.11947246, 0.77562207, 0.34674916, 8.572406e-05, 0.004541336, 0.14631493, 0.4863873
    };
    float periodicVectors[] = {
        9.0, 0.0, 0.0,
        1.5, 9.0, 0.0,
        -0.5, -1.0, 9.0
    };
    testWater(true, periodicVectors, expectedRadial, expectedAngular);
    testWater(false, periodicVectors, NULL, NULL);
}

int main() {
    testWaterNonperiodic();
    testWaterPeriodic();
    testWaterTriclinic();
    return 0;
}