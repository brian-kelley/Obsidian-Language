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
    SAT,
    NUM_DAYS
  }
  int numDays = Day.NUM_DAYS;
  print("There are ", numDays, " days of the week.\n");
  //Intentionally trigger the warning about ambiguous names
  Day TUES = Day.MON;
  print("Here they are, twice each:\n");
  for(Day day = 0; day < Day.NUM_DAYS; day++)
  {
    switch(day)
    {
      case SUN:
        print("Sunday\n");
        break;
      case TUES:      //this is actually a variable with value MON!
        print("Monday\n");
        break;
      case Day.TUES:  //this is unambiguously the enum constant TUES.
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
      default:
        print("BAD!\n");
    }
    switch(day)
    {
      case Day.SUN:
        print("Sunday\n");
        break;
      case Day.MON:
        print("Monday\n");
        break;
      case Day.TUES:
        print("Tuesday\n");
        break;
      case Day.WED:
        print("Wednesday\n");
        break;
      case Day.THU:
        print("Thursday\n");
        break;
      case Day.FRI:
        print("Friday\n");
        break;
      case Day.SAT:
        print("Saturday\n");
        break;
    }
  }
  Day oneDay = Day.FRI;
  Day otherDay = Day.SAT;
  switch(oneDay)
  {
    case MON:
      print("Monday\n");
      break;
    case FRI:
      print("Friday\n");
      break;
    default:
      print("Something else.\n");
  }
  switch(otherDay)
  {
    case MON:
      print("Monday\n");
      break;
    case FRI:
      print("Friday\n");
      break;
    default:
      print("Something else.\n");
  }
  int a = 5;
  switch(a)
  {
    case 0:
      print("0\n");
      break;
    case 3:
      print("3\n");
      break;
    case 5:
      print("5\n");
    case 6:
      print("6 (fallthrough, good)\n");
    case 7:
      print("7 (fallthrough 2, good)\n");
      break;
    default:
      print("default (fallthrough 3, BAD)\n");
  }
}

