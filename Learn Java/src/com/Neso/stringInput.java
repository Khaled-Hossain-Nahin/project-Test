/*
input.next(); for a single world
input.nextLine(); for a Line
 input.nextInt();
input.nextDouble();
input.nextByte();
input.nextShort();
input.nextShort()
input.nextBoolean();
*/

package com.Neso;
import java.util.Scanner;

public class stringInput {
    static void main() {
        bool();
    }
    static void inputs(){
        Scanner a =  new Scanner(System.in);
        int b = a.nextInt();
        System.out.println((b + 10));
        System.out.println((a.nextInt() + " times"));
    }
    static void bool(){
        Scanner input=new Scanner(System.in);
        if(input.nextBoolean())
            System.out.println("AShol");
        else
            System.out.println("Nokol");


    }
    static void string_input() {
        String us = "Ali";
        Scanner input = new Scanner(System.in);
        System.out.println("Age:" + input.next());

        Scanner name = new Scanner(System.in);
        System.out.println("Full name:" + name.nextLine());

    }

}
