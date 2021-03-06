////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Copyright © 2019, UChicago Argonne, LLC
//
//All Rights Reserved
//
//Software Name: ptychopy
//
//By: Argonne National Laboratory
//
//OPEN SOURCE LICENSE
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
//following conditions are met:
//
//1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//disclaimer in the documentation and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
//derived from this software without specific prior written permission.
//
//DISCLAIMER
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
//INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Python.h>
#include <numpy/arrayobject.h>
//#include "SimplePhaser.h"
#include "MPIPhaser.h"
#include "Parameters.h"
#include "utilities.h"
#include "Cuda2DArray.hpp"
#include "CudaSmartPtr.h"
#include "Sample.h"
#include "Probe.h"

#include <iostream>

#define PY_ARRAY_UNIQUE_SYMBOL

using namespace std;

/* Define module and function docstrings */
static char module_docstring[] = "This module provides an interface to ptycholib GPU-accelerated phase retrieval.";
static char helloworld_docstring[] = "This function exists for debugging and testing purposes.";

static char epie_docstring[] = "This function performs iterative phase retrieval using the ePIE algorithm with keyword.";
static char dm_docstring[] = "This function performs iterative phase retrieval using the dm algorithm with keyword.";
static char mls_docstring[] = "This function performs iterative phase retrieval using the mls algorithm with keyword.";

static char epiecmdstr_docstring[] = "This function performs iterative phase retrieval using the ePIE algorithm \
            with a whole string.";
static char dmcmdstr_docstring[] = "This function performs iterative phase retrieval using the DM algorithm \
            with a whole string.";
static char mlscmdstr_docstring[] = "This function performs iterative phase retrieval using the MLs algorithm \
            with a whole string.";

static char epieinit_docstring[] = "This function performs the init.";
static char epiepost_docstring[] = "This function performs the post.";
static char epiestep_docstring[] = "This function performs the step.";
static char epieresobj_docstring[] = "This function returns the object of the reconstruction.";
static char epieresprobe_docstring[] = "This function returns the probe of the reconstruction.";

/* Define function headers */
static PyObject *ptycholib_helloworld(PyObject *self, PyObject *args);
static PyObject *ptycholib_epie(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_dm(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_mls(PyObject *self, PyObject *args, PyObject *keywds);


static PyObject *ptycholib_epiecmdstr(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_dmcmdstr(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_mlscmdstr(PyObject *self, PyObject *args, PyObject *keywds);

static PyObject *ptycholib_epieinit(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_epiepost(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_epiestep(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_epieresobj(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *ptycholib_epieresprobe(PyObject *self, PyObject *args, PyObject *keywds);

static IPhaser* phaser;

static unsigned int istep = 0;

/* Define module methods */
static PyMethodDef module_methods[] = {
    {"helloworld", (PyCFunction)ptycholib_helloworld, METH_VARARGS, helloworld_docstring},
    {"epie", (PyCFunction)ptycholib_epie, METH_VARARGS|METH_KEYWORDS, epie_docstring},
    {"dm", (PyCFunction)ptycholib_dm, METH_VARARGS|METH_KEYWORDS, dm_docstring},
    {"mls", (PyCFunction)ptycholib_mls, METH_VARARGS|METH_KEYWORDS, mls_docstring},
    {"epiecmdstr", (PyCFunction)ptycholib_epiecmdstr, METH_VARARGS|METH_KEYWORDS, epiecmdstr_docstring},
    {"dmcmdstr", (PyCFunction)ptycholib_dmcmdstr, METH_VARARGS|METH_KEYWORDS, dmcmdstr_docstring},
    {"mlscmdstr", (PyCFunction)ptycholib_mlscmdstr, METH_VARARGS|METH_KEYWORDS, mlscmdstr_docstring},
    {"epieinit", (PyCFunction)ptycholib_epieinit, METH_VARARGS|METH_KEYWORDS, epieinit_docstring},
    {"epiepost", (PyCFunction)ptycholib_epiepost, METH_VARARGS|METH_KEYWORDS, epiepost_docstring},
    {"epiestep", (PyCFunction)ptycholib_epiestep, METH_VARARGS|METH_KEYWORDS, epiestep_docstring},
    {"epieresobj", (PyCFunction)ptycholib_epieresobj, METH_VARARGS|METH_KEYWORDS, epieresobj_docstring},
    {"epieresprobe", (PyCFunction)ptycholib_epieresprobe, METH_VARARGS|METH_KEYWORDS, epieresprobe_docstring},
    {NULL, NULL, 0, NULL}
};

/* Define the module and it's methods */
static struct PyModuleDef ptycholib_module = {
    PyModuleDef_HEAD_INIT,
    "ptychopy", /* Name of module */
    module_docstring, /* Module docstring */
    -1, /* Size of per-interpreter state of the module or -1 if module keeps state in global variables*/
    module_methods
};

/* Init the module */
PyMODINIT_FUNC PyInit_ptychopy(void)
{
    PyObject *m = PyModule_Create(&ptycholib_module);

    if (m==NULL)
        return NULL;
    /* Load numpy functionality */
    import_array();

    return m;
}

/* Function definitions */
static PyObject *ptycholib_helloworld(PyObject *self, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    printf("Hello, World!");

    Py_RETURN_NONE;
}

static void ptychopy_algorithm(PyObject *args, PyObject *keywds, char *algorithm)
{
    // Default value for the algorithm parameters
    char *jobID="";
    char *fp="";
    int fs=0;
    char* hdf5path="";
    int dpf = 1;
    double beamSize=400e-9;
    char *probeGuess="";
    char *objectGuess="";
    int size=512;
    int qx=128;
    int qy=128;
    int nx=256;
    int ny=256;
    int scanDimsx=26;
    int scanDimsy=26;
    int spiralScan=0;
    int flipScanAxis=0;
    int mirror1stScanAxis=0;
    int mirror2ndScanAxis=0;
    double stepx=40e-9;
    double stepy=40e-9;
    int probeModes=1;
    double lambd=2.3843e-10;
    double dx_d=172e-6;
    double z=2.2;
    int iter=100;
    int T=0;
    int jitterRadius=0;
    double delta_p=0.1;
    int threshold=0;
    int rotate90=0;
    int sqrtData=0;
    int fftShiftData=0;
    int binaryOutput=0;
    int simulate=0;
    int phaseConstraint=1;
    int updateProbe=10;
    int updateModes=20;
    int beamstopMask=0;
    char *lf="";
    int PPS=20;

    static char *kwlist[] = {"jobID", "fp", "fs", "hdf5path", "dpf", "beamSize", "probeGuess", "objectGuess", \
    "size", "qx", "qy", "nx", "ny", "scanDimsx", "scanDimsy", "spiralScan", "flipScanAxis", "mirror1stScanAxis", \
    "mirror2ndScanAxis", "stepx", "stepy", "probeModes", "lambd", "dx_d", "z", "iter", "T", "jitterRadius", \
    "delta_p",  "threshold", "rotate90", "sqrtData", "fftShiftData", "binaryOutput", "simulate", \
    "phaseConstraint", "updateProbe", "updateModes", "beamstopMask", "lf", "PPS", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ssisidssiiiiiiiiiiiddidddiiidiiiiiiiiiisi", kwlist,
                &jobID, &fp, &fs, &hdf5path, &dpf, &beamSize, &probeGuess, &objectGuess, \
                &size, &qx, &qy, &nx, &ny, &scanDimsx, &scanDimsy, &spiralScan, &flipScanAxis, &mirror1stScanAxis, \
                &mirror2ndScanAxis, &stepx, &stepy, &probeModes, &lambd, &dx_d, &z, &iter, &T, &jitterRadius, \
                &delta_p,  &threshold, &rotate90, &sqrtData, &fftShiftData, &binaryOutput, &simulate, \
                &phaseConstraint, &updateProbe, &updateModes, &beamstopMask, &lf, &PPS))

    printf("Running ePIE.\n");
//    printf("the jobID is %s \n", jobID);
////    printf("the fp is %s \n", fp);
//    printf("the beamSize is %.8e \n", beamSize);
//    printf("the iter is %d \n", iter);
//    printf("the scanDims is %d, %d, stepx is %.8e, stepy is %.8e, size is %d, dx_d is %.8e, z is %.8e  \n", scanDimsx, scanDimsy, stepx, stepy, size, dx_d, z);
//    printf("the lambd is %.8e \n", lambd);
//    printf("the algorithm is %s, the simulate is %d \n", algorithm, simulate);

    CXParams::getInstance()->parseFromCPython(jobID, algorithm, fp, fs, hdf5path, dpf, beamSize, probeGuess, objectGuess, \
                size, qx, qy, nx, ny, scanDimsx, scanDimsy, spiralScan, flipScanAxis, mirror1stScanAxis, \
                mirror2ndScanAxis, stepx, stepy, probeModes, lambd, dx_d, z, iter, T, jitterRadius, \
                delta_p,  threshold, rotate90, sqrtData, fftShiftData, binaryOutput, simulate, \
                phaseConstraint, updateProbe, updateModes, beamstopMask, lf, PPS);

    IPhaser* phaser = new IPhaser;
    if(phaser->init())
	{
		phaser->addPhasingMethod( 	CXParams::getInstance()->getReconstructionParams()->algorithm.c_str(),
									CXParams::getInstance()->getReconstructionParams()->iterations);
		phaser->phase();
		phaser->writeResultsToDisk();
	}
	delete phaser;
}

static PyObject *ptycholib_epie(PyObject *self, PyObject *args, PyObject *keywds)
{

    ptychopy_algorithm(args, keywds, "ePIE");

//    complex_t* h_objectArray = phaser->getSample()->getObjectArray()->getHostPtr<complex_t>();
//    delete phaser;
//
//    npy_intp dims[2] = {2048,2048};
//    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_objectArray);

//    PyArrayObject *recon_ob = NULL;
//
//    if (recon_ob==NULL)
//    {
//        return NULL;
//    }
//
//    Py_INCREF(recon_ob);
//    return PyArray_Return(recon_ob);
    return Py_True;
}
static PyObject *ptycholib_dm(PyObject *self, PyObject *args, PyObject *keywds)
{

    ptychopy_algorithm(args, keywds, "DM");

    return Py_True;
}

static PyObject *ptycholib_mls(PyObject *self, PyObject *args, PyObject *keywds)
{

    ptychopy_algorithm(args, keywds, "MLs");

    return Py_True;
}

static size_t makeargs(char **ap, size_t n, char *s)
{
    int c, inarg = 0;
    size_t save;
    for (save = n--; (c = (unsigned char) *s) && n; s++)
        if (inarg && isspace(c))
        {
            *s = '\0';
            n--;
            inarg = 0;
        } else if (!inarg && !isspace(c))
        {
            *ap++ = s;
            inarg = 1;
        }
    *ap = 0;
    return save - n;
}

static size_t countargs(const char *s)
{
    int inarg = 0, c;
    size_t n = 0;


    while ((c = (unsigned char) *s++))
        if (inarg && isspace(c)) {
            inarg = 0;
        } else if (!inarg && !isspace(c)) {
            n++;
            inarg = 1;
        }
    return n;
}

static PyObject *ptycholib_epiecmdstr(PyObject *self, PyObject *args, PyObject *keywds)
{

    /* io */
    int io_threads=100;
    char *filename_pattern = "";
    int start_file_number = 1501;
    /* preprocessing */
    int size = 512;
    int threshhold = 0;
    int rotate_90 = 0;
    int sqrt_data = 0;
    int fftshift_data = 0;
    int flip_scan_axis = 0;
    /* experiment */
    float beam_size=400e-9;
    float energy=5.0;
    float lambda=1.24e-9/energy;
    float dx=172e-6;
    float z=1.0;
    int simulate=1;
    /* cartesian scan */
    int scan_dims_0=55;
    int scan_dims_1=75;
    float step_0=40e-9;
    float step_1=40e-9;
    /* probe guess */
    char *probeGuess = "";
    /* phase retrieval */
    int object_array=2048;
    int i=10;
    char *job_id="gentest101";
    int qx0 = 243;
    int qy0 = 153;
    int probe_modes = 1;
    int position_jitter_radius=0;
    int share_frequency = 0;
    int dpf = 1;

    char *cmdstr = "";
//    char cmdstr[1000];
    static char *kwlist[] = {"cmdstr", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|s", kwlist, &cmdstr))
        return NULL;
    lambda = 1.24e-9/energy;
    char str[1000];
    printf("the command line string is %s \n", cmdstr);
// Make the command line parameters
    size_t len = countargs(cmdstr) + 1;
    char **r;
    if (!(r = (char **)malloc(len * sizeof *r)))
        return 0;
    makeargs(r, len, cmdstr);


    CXParams::getInstance()->parseFromCommandLine(len-1, r);
    IPhaser* phaser = new IPhaser;
    if(phaser->init())
    {
        phaser->addPhasingMethod("ePIE", CXParams::getInstance()->getReconstructionParams()->iterations);
        phaser->phase();
//        phaser->phaseVisStep();
    }

    phaser->writeResultsToDisk();
    complex_t* h_objectArray = phaser->getSample()->getObjectArray()->getHostPtr<complex_t>();
    int x = phaser->getSample()->getObjectArray()->getX();
    int y = phaser->getSample()->getObjectArray()->getY();

//    delete phaser;

    npy_intp dims[2] = {x,y};
    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_objectArray);
    Py_INCREF(recon_ob);
    return PyArray_Return(recon_ob);
//    return Py_False;
}

static PyObject *ptycholib_dmcmdstr(PyObject *self, PyObject *args, PyObject *keywds)
{

    /* io */
    int io_threads=100;
    char *filename_pattern = "";
    int start_file_number = 1501;
    /* preprocessing */
    int size = 512;
    int threshhold = 0;
    int rotate_90 = 0;
    int sqrt_data = 0;
    int fftshift_data = 0;
    int flip_scan_axis = 0;
    /* experiment */
    float beam_size=400e-9;
    float energy=5.0;
    float lambda=1.24e-9/energy;
    float dx=172e-6;
    float z=1.0;
    int simulate=1;
    /* cartesian scan */
    int scan_dims_0=55;
    int scan_dims_1=75;
    float step_0=40e-9;
    float step_1=40e-9;
    /* probe guess */
    char *probeGuess = "";
    /* phase retrieval */
    int object_array=2048;
    int i=10;
    char *job_id="gentest101";
    int qx0 = 243;
    int qy0 = 153;
    int probe_modes = 1;
    int position_jitter_radius=0;
    int share_frequency = 0;
    int dpf = 1;

    char *cmdstr = "";
//    char cmdstr[1000];
    static char *kwlist[] = {"cmdstr", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|s", kwlist, &cmdstr))
        return NULL;
    lambda = 1.24e-9/energy;
    char str[1000];
    printf("the command line string is %s \n", cmdstr);
// Make the command line parameters
    size_t len = countargs(cmdstr) + 1;
    char **r;
    if (!(r = (char **)malloc(len * sizeof *r)))
        return 0;
    makeargs(r, len, cmdstr);


    CXParams::getInstance()->parseFromCommandLine(len-1, r);
    IPhaser* phaser = new IPhaser;
    if(phaser->init())
    {
        phaser->addPhasingMethod("DM", CXParams::getInstance()->getReconstructionParams()->iterations);
        phaser->phase();
//        phaser->phaseVisStep();
    }

    phaser->writeResultsToDisk();
    complex_t* h_objectArray = phaser->getSample()->getObjectArray()->getHostPtr<complex_t>();
    int x = phaser->getSample()->getObjectArray()->getX();
    int y = phaser->getSample()->getObjectArray()->getY();

//    delete phaser;
    npy_intp dims[2] = {x,y};
    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_objectArray);
    Py_INCREF(recon_ob);
    return PyArray_Return(recon_ob);
//    return Py_False;
}

static PyObject *ptycholib_mlscmdstr(PyObject *self, PyObject *args, PyObject *keywds)
{

    /* io */
    int io_threads=100;
    char *filename_pattern = "";
    int start_file_number = 1501;
    /* preprocessing */
    int size = 512;
    int threshhold = 0;
    int rotate_90 = 0;
    int sqrt_data = 0;
    int fftshift_data = 0;
    int flip_scan_axis = 0;
    /* experiment */
    float beam_size=400e-9;
    float energy=5.0;
    float lambda=1.24e-9/energy;
    float dx=172e-6;
    float z=1.0;
    int simulate=1;
    /* cartesian scan */
    int scan_dims_0=55;
    int scan_dims_1=75;
    float step_0=40e-9;
    float step_1=40e-9;
    /* probe guess */
    char *probeGuess = "";
    /* phase retrieval */
    int object_array=2048;
    int i=10;
    char *job_id="gentest101";
    int qx0 = 243;
    int qy0 = 153;
    int probe_modes = 1;
    int position_jitter_radius=0;
    int share_frequency = 0;
    int dpf = 1;

    char *cmdstr = "";
//    char cmdstr[1000];
    static char *kwlist[] = {"cmdstr", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|s", kwlist, &cmdstr))
        return NULL;
    lambda = 1.24e-9/energy;
    char str[1000];
    printf("the command line string is %s \n", cmdstr);
// Make the command line parameters
    size_t len = countargs(cmdstr) + 1;
    char **r;
    if (!(r = (char **)malloc(len * sizeof *r)))
        return 0;
    makeargs(r, len, cmdstr);


    CXParams::getInstance()->parseFromCommandLine(len-1, r);
    IPhaser* phaser = new IPhaser;
    if(phaser->init())
    {
        phaser->addPhasingMethod("MLs", CXParams::getInstance()->getReconstructionParams()->iterations);
        phaser->phase();
//        phaser->phaseVisStep();
    }

    phaser->writeResultsToDisk();
    complex_t* h_objectArray = phaser->getSample()->getObjectArray()->getHostPtr<complex_t>();
    int x = phaser->getSample()->getObjectArray()->getX();
    int y = phaser->getSample()->getObjectArray()->getY();

//    delete phaser;

    npy_intp dims[2] = {x,y};
    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_objectArray);
    Py_INCREF(recon_ob);
    return PyArray_Return(recon_ob);
//    return Py_False;
}

static PyObject *ptycholib_epieinit(PyObject *self, PyObject *args, PyObject *keywds)
{

    char *cmdstr = "";

//    if(phaser != NULL)
//        delete phaser;
//
    phaser = new IPhaser;
    istep = 0;
    static char *kwlist[] = {"cmdstr", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|s", kwlist,&cmdstr))
        return NULL;

//    char str[1000];
//    printf("the command line string is %s \n", cmdstr);

// Make the command line parameters
//    size_t len = countargs(cmdstr) + 1;
//    char **r;
//
//    if (!(r = (char **)malloc(len * sizeof *r)))
//        return 0;
//
//    makeargs(r, len, cmdstr);

    size_t len = countargs(cmdstr) + 1;
    char **r;

    if (!(r = (char **)malloc((len+1) * sizeof *r)))
        return 0;

    if (!(*r = (char *)malloc(strlen(cmdstr) + 1)))
    {
        free(r);
        return 0;
    }

    makeargs(r+1, len, strcpy(*r, cmdstr));
    CXParams::getInstance()->parseFromCommandLine(len-1, r+1);
    if(phaser->init())
    {
        phaser->addPhasingMethod("ePIE", CXParams::getInstance()->getReconstructionParams()->iterations);
        phaser->phaseinit();
    }

    free(*r);
    free(r);
    return Py_True;

}

static PyObject *ptycholib_epiepost(PyObject *self, PyObject *args, PyObject *keywds)
{

    phaser->phasepost();
    phaser->writeResultsToDisk();
    if(phaser != NULL)
        delete phaser;
    return Py_True;

}

static PyObject *ptycholib_epiestep(PyObject *self, PyObject *args, PyObject *keywds)
{

    istep++;
    phaser->phasestepvis(istep);
    return Py_True;

}

static PyObject *ptycholib_epieresobj(PyObject *self, PyObject *args, PyObject *keywds)
{

    complex_t* h_objectArray = phaser->getSample()->getObjectArray()->getHostPtr<complex_t>();
    int x = phaser->getSample()->getObjectArray()->getX();
    int y = phaser->getSample()->getObjectArray()->getY();

    npy_intp dims[2] = {x,y};
    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_objectArray);
    Py_INCREF(recon_ob);
    return PyArray_Return(recon_ob);

}

static PyObject *ptycholib_epieresprobe(PyObject *self, PyObject *args, PyObject *keywds)
{

    complex_t* h_probeArray = phaser->getProbe()->getModes()->getPtr()->getHostPtr<complex_t>();
    int x = phaser->getProbe()->getModes()->getPtr()->getX();
    int y = phaser->getProbe()->getModes()->getPtr()->getY();

    npy_intp dims[2] = {x,y};
    PyArrayObject *recon_ob = (PyArrayObject*)PyArray_SimpleNewFromData(2, dims, NPY_CFLOAT, h_probeArray);
    Py_INCREF(recon_ob);
    return PyArray_Return(recon_ob);

}
