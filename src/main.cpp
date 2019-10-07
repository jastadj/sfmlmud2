#include <cstdlib>
#include "mud.hpp"

int main(int argc, char *argv[])
{
    Mud *mud = Mud::getInstance();
    mud->start();

    return 0;
}
