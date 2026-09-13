package com.Neso;
import java.util.Scanner;

public class if_else_prac1 {
    static void main() {
        calculaor();
    }

    static void calculaor() {
        System.out.println("Enter n1 op n2: ");
        Scanner n= new Scanner(System.in);
        double a = n.nextDouble();
        char op = n.next().charAt(0);
        double b = n.nextDouble();

        if(op=='+')
            System.out.println(a+b);
        else if (op=='-')
            System.out.println(a-b);
        else if (op=='*')
            System.out.println(a*b);
        else
            System.out.println(a/b);


    }
}
