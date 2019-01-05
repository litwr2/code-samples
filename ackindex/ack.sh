ack() {
  case $1 in
    0) echo $(($3 + 1));;
    *) case $3 in
         0) case $1 in
              1) echo $2;;
              2) echo 0;;
              *) echo 1;;
            esac;;
         *) echo $(ack $(($1 - 1)) $2 $(ack $1 $2 $(($3 - 1))));;
       esac;;
  esac
}

ack 1 1 270


