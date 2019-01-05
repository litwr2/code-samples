#RUN IT BY
#   time bash fib.sh

fib() {
  case $1 in
    [12]) echo 1;;
    *) echo $(($(fib $(($1 - 1))) + $(fib $(($1 - 2)))));;
  esac
}

fib 15


