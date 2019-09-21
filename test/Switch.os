proc void main()
{
  enum Day
  {
    SUN,
    MON,
    TUES,
    WED,
    THU,
    FRI,
    SAT
  }
  for(int day = 0; day < 7; day++)
  {
    switch(day)
    {
      case SUN:
        print("Sunday\n");
        break;
      case MON:
        print("Monday\n");
        break;
      case TUES:
        print("Tuesday\n");
        break;
      case WED:
        print("Wednesday\n");
        break;
      case THU:
        print("Thursday\n");
        break;
      case FRI:
        print("Friday\n");
        break;
      case SAT:
        print("Saturday\n");
        break;
    }
  }
}
