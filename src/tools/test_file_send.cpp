#include <iostream>
#include "pkgs/file_send.h"

using namespace ff;
int main(void)
{
  file_send("../confs/slave_net_conf.ini","211.69.198.47","/home/sherry");
}