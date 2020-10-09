cmd_/home/lmi/workspace/csel1/Modules_Ex/modules.order := {   echo /home/lmi/workspace/csel1/Modules_Ex/mymodule.ko; :; } | awk '!x[$$0]++' - > /home/lmi/workspace/csel1/Modules_Ex/modules.order
