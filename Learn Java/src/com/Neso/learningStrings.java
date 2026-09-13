package com.Neso;

import java.util.Scanner;

public class learningStrings {
    static void main(String[] args) {
        string_input();
    }

    static void input(){

    }

    static void string_methods() {
        String str= "This is a MADHOUSE";
//        String us=str.toUpperCase(),up=str.toLowerCase();
//        System.out.println(us);
//        System.out.println(up);

        //length
        System.out.print("Length:");
        int len = str.length();
        System.out.println(len);

        System.out.println(str.isEmpty());
        System.out.println(str.isBlank());
        //isEmpty() checks if the string length is exactly 0,
        // while isBlank() checks if the string is either empty
        // or contains only whitespace characters

        //charAT()
        char c = str.charAt(0);
        System.out.println(c);

        //Getting the index of a char
        System.out.println(str.indexOf('h'));
        System.out.println(str.lastIndexOf('h'));

        //concat
        String s1= "Hello ";
        String s2= "World ";

        System.out.println(s1 + s2);
        System.out.println(s1 + s2 + 5 + " Times");
    }

    static void string_input() {
        String us = "Ali";
        Scanner input = new Scanner(System.in);
        System.out.println("Age:" + input.next());

        Scanner name = new Scanner(System.in);
        System.out.println("Full name:" + name.nextLine());

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

    }
}
