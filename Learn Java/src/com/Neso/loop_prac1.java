package com.Neso;
import java.lang.classfile.instruction.SwitchCase;
import java.util.ArrayList;
import java.util.Scanner;
public class loop_prac1 {
    static void main() {
       while(true)
           rev();
    }
    static void checker() {
        Scanner x =new Scanner(System.in);
        while(true){
            int a =x.nextInt();
            if(a<=10 && a>=1){
                System.out.println("Good!");
                break;
            }
            else
                System.out.println("Try Again!!");

        }
    }
    static void even() {
        Scanner x = new Scanner(System.in);

        for(int i=2;i<=100;i+=2){
            System.out.print(i + " ");
        }
    }
    static void till100() {
        Scanner x = new Scanner(System.in);
        int a = x.nextInt();
        int sum=a;
        while(sum<=100){
            System.out.println(a);
            a = x.nextInt();
            sum+=a;
        }
        System.out.println("Exceeded");

    }
    static void divisors() {
        Scanner x = new Scanner(System.in);
        int n = x.nextInt();

        int sum=0;
        for(int i=1;i<=n/2;i++){
            if(n%i==0){
                sum+=i;
            }
        }
        System.out.println(sum);
    }
    static void C_is_better(){
        Scanner x = new Scanner(System.in);
        String n = x.next();
        int val=0;
        for(int i=0;i<n.length();i++){
             val += n.charAt(i) - '0';
        }
        System.out.println(val);
    }
    static void extra_space(){
       Scanner x = new Scanner(System.in);
       String s= x.nextLine();

       for(int i=0;i<s.length();i++){
           System.out.print(s.charAt(i) + " ");
       }
    }

    static void rev(){
        Scanner x = new Scanner(System.in);
        String s= x.nextLine();

        for(int i=s.length()-1;i>=0;i--){
            System.out.print(s.charAt(i) + " ");
        }
    }

}
