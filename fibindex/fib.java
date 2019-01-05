//RUN IT BY
//   javac fib.java;time java fib

public class fib {
    static int fibc(int n) {
      if (n < 3)
        return 1;
      return fibc(n - 1) + fibc(n - 2);
    }
    public static void main(String[] args) {
        System.out.println(fibc(40)); //31 - GNU
    }
}
