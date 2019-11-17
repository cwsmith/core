#include <PCU.h>
#include <lionPrint.h>
#include <apfMDS.h>
#include <gmi.h>
#include <gmi_mesh.h>
#include <gmi_null.h>
#include <apf.h>
#include <apfConvert.h>
#include <apfMesh2.h>
#include <apfNumbering.h>
#include <ma.h>
#include <pcu_util.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <getopt.h>

const char* gmi_path = NULL;
const char* in_mesh_path = NULL;
const char* vtk_path = NULL;
int should_reorder = 0;

void getConfig(int argc, char** argv) {

  opterr = 0;

  static struct option long_opts[] = {
    {"reorder", no_argument, &should_reorder, 1},
    {0, 0, 0, 0}  // terminate the option array
  };

  const char* usage=""
    "[options] <model file> <input mesh> <vtk prefix>\n"
    "options:\n"
    "  --reorder  reorder the mesh\n";

  int option_index = 0;
  while(1) {
    int c = getopt_long(argc, argv, "", long_opts, &option_index);
    if (c == -1) break; //end of options
    switch (c) {
      case 0: // reorder
        break;
      case '?':
        if (!PCU_Comm_Self())
          printf ("warning: skipping unrecognized option\n");
        break;
      default:
        if (!PCU_Comm_Self())
          printf("Usage %s %s", argv[0], usage);
        exit(EXIT_FAILURE);
    }
  }

  if(argc-optind != 3) {
    if (!PCU_Comm_Self())
      printf("Usage %s %s", argv[0], usage);
    exit(EXIT_FAILURE);
  }
  int i=optind;
  gmi_path = argv[i++];
  in_mesh_path = argv[i++];
  vtk_path = argv[i++];

  if (!PCU_Comm_Self()) {
    printf ("reorder %d\n", should_reorder);
    printf ("model \'%s\' input mesh \'%s\' output vtk \'%s\'\n",
      gmi_path, in_mesh_path, vtk_path);
  }
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  PCU_Comm_Init();
  lion_set_verbosity(1);

  gmi_register_mesh();
  gmi_register_null();

  getConfig(argc, argv);

  double t1 = PCU_Time();
  gmi_model* mdl = gmi_load(gmi_path);
  apf::Mesh2* apfMesh = apf::loadMdsMesh(mdl,in_mesh_path);
  double t2 = PCU_Time();
  if(!PCU_Comm_Self())
    fprintf(stderr, "created the apf mesh in %f seconds\n", t2-t1);
  apf::Mesh2* mesh = apf::createMdsMesh(mdl, apfMesh, should_reorder);
  double t3 = PCU_Time();
  if(!PCU_Comm_Self())
    fprintf(stderr, "created the apf_mds mesh in %f seconds\n", t3-t2);

  apf::disownMdsModel(apfMesh);
  apf::destroyMesh(apfMesh);
  apf::printStats(mesh);
  mesh->verify();
  apf::writeVtkFiles(vtk_path, mesh);

  mesh->destroyNative();
  apf::destroyMesh(mesh);

  PCU_Comm_Free();
  MPI_Finalize();
}
