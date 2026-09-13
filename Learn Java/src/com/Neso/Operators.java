package com.Neso;
import java.util.Scanner;

public class Operators {
    static void main() {
        pre_post_increment();
    }

    static void pre_post_increment() {
        int i=1;
        int j=++i;
        System.out.println("i:" + i +",J: " + j);

        int a=1;
        int b=a++;
        System.out.println("a:" + a +",b: " + b);
    }

    static void realtional() {
        boolean b1 = (1==1);//true
        boolean b2 = (5==1);//false

        boolean b3 = (5!=1);//true
        boolean b4 = (1!=1);//false

        boolean b5=(5>2); // true
        boolean b6=(5<2); // false

        boolean bx= true;
        boolean by = !bx; //false
    }

    static void logical() {

    }

}
