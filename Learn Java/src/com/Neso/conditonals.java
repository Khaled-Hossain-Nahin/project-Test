package com.Neso;
import java.util.Scanner;


public class conditonals {
    static void main() {
        switch_cases();
    }
    static void tarnary() {
        int a =10, b=0;

        int mx= a>b ?a:b;

        System.out.println(a>b ?a:b);
        System.out.println(mx);

    }
    static void switch_cases() {
        Scanner x = new Scanner(System.in);
        int n=x.nextInt();

        switch (n){
            case 10: System.out.println("=10");
            case 90: System.out.println("=90");
            case 100: System.out.println("=100");
            default: System.out.println("None");
        // if one is true then all the below gets executed
        // use break; to avoid this
        }
    }
}
