#include <iostream>
#include <steam_controller/steam_controller.hpp>

int main(int argc, char** argv)
{
  steam_controller::context context;
  auto all = context.enumerate();
  return 0;
}
