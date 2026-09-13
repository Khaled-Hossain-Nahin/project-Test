package com.Neso;
import java.util.ArrayList;
import java.util.Scanner;

public class if_else_prac2 {
    static void main() {
        lucky();
    }

    static void lucky() {
        Scanner x = new Scanner(System.in);
        String s = x.next();

        ArrayList<Integer>vec=new ArrayList<>();

        for(int i=0;i<4;i++){
            int val = s.charAt(i) - '0';
            vec.add(val);
        }

        if (vec.get(0)+vec.get(1)==vec.get(2)+vec.get(3))
            System.out.println("Lucky");
        else System.out.println("Unlucky");

    }
}
