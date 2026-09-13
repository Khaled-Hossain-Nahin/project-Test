package com.Neso;

public class Dragon {
    public static void main(String[] args) {
        variables();
    }

    static void sayName() {
        System.out.println("Khaled Hossain");
    }


    //String
     static void sayAge() {
        String a ="Dragons";
        System.out.println(19);
        System.out.println(a);
    }

    //int data
    static void variables() {
        int i1= 100;
        i1++;
        System.out.println(i1);

        double d;
        System.out.println(d=11.5);
    }
    static void bytes() {
        byte b1 = -128, b2 = 127;// mx & mn range
        //byte error1 = -129,error1 = 128 ;
    }

    static void sizePrinting() {
        //Integer
        System.out.println(Integer.MAX_VALUE);
        System.out.println(Integer.MIN_VALUE);

        System.out.println(Short.MAX_VALUE);

        System.out.println(Long.MAX_VALUE);
        System.out.println(Long.MIN_VALUE);

    }

    static void point() {
        float f=6F;
        double d = 4.1454411444;

        System.out.println(f);
        System.out.println(d);
    }

    static void character() {
        System.out.println("Printing Characters: ");
        char c1='A',c2=65,c3='\u0041';
        System.out.println(c1);
        System.out.println(c2);
        System.out.println(c3);

        System.out.println("Printing Characters using int: ");
        int a='a';
        System.out.println(a);
    }

    static void bool() {
        boolean b1=false;
        if(b1) System.out.println("OK");
        else System.out.println("Not OK");
    }



}
