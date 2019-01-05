//RUN IT BY
//   javac ack.java;time java ack

public class ack {
    static int ackc(int n, int x, int y) {
      if (n == 0) return y + 1;
      if (y == 0) {
         if (n == 1) return x;
         if (n == 2) return 0;
         return 1;
      }
      return ackc(n - 1, y, ackc(n, x, y - 1));
    }
    public static void main(String[] args) {
        System.out.println(ackc(1, 1, 8000));
    }
}
