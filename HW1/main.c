#include "stdio.h"

int main()
{
  float user_farenheit;
  
  printf("Enter a temparature in farenheit: ");
  scanf("%f", &user_farenheit);
  
  float user_celsius; 
  user_celsius = (user_farenheit - 32) * 5/9;
  printf("Your temparature in farenheit: %f\n", user_farenheit);
  printf("Your temparature in celsius: %f\n", user_celsius);

  return 0;
}
