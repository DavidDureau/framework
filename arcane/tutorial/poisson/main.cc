#include <iostream>
#include <vector>
#include <arcane/impl/ArcaneMain.h>

using namespace Arcane;

int
main(int argc,char* argv[])
{
  int r = 0;
  ArcaneMain::arcaneInitialize();
  {
    ApplicationInfo app_info(&argc,&argv,"Poisson",VersionInfo(1,0,0));
    r = ArcaneMain::arcaneMain(app_info);
  }
  ArcaneMain::arcaneFinalize();
  return r;
}
