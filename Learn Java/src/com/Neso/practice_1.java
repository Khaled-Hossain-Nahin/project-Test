package com.Neso;
import java.util.Scanner;

public class practice_1 {
    static void main() {
        prac2();
    }

    static void prac1() {
        System.out.println("Fav int: ");
        Scanner x = new Scanner(System.in);
        System.out.println(x.nextInt()+ " is fav int");
    }

    static void prac2() {
        System.out.print("Name & Age: ");
        Scanner x = new Scanner(System.in);
       // Scanner y = new Scanner(System.in);
        System.out.println(x.next()+ "! you are " + x.nextFloat() + " Years Old");
    }

}
