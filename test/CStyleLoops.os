proc main: void()
{
  {
    //init, cond, incr
    int pyramid = 0;
    for(int i = 1; i <= 5; i++)
    {
      pyramid += i * i;
    }
    print(pyramid, '\n');
  }
  {
    //cond, incr
    int pyramid = 0;
    int i = 1;
    for(; i <= 5; i++)
    {
      pyramid += i * i;
    }
    print(pyramid, '\n');
  }
  {
    //init, incr
    int pyramid = 0;
    for(int i = 1;; i++)
    {
      pyramid += i * i;
      if(i == 5)
        break;
    }
    print(pyramid, '\n');
  }
  {
    //init, cond
    int pyramid = 0;
    for(int i = 1; i <= 5;)
    {
      pyramid += i * i;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //init
    int pyramid = 0;
    for(int i = 1;;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //cond
    int pyramid = 0;
    int i = 1;
    for(; i <= 5;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //incr
    int pyramid = 0;
    int i = 1;
    for(;; i++)
    {
      pyramid += i * i;
      if(i == 5)
        break;
    }
    print(pyramid, '\n');
  }
  {
    //none
    int pyramid = 0;
    int i = 1;
    for(;;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //while
    int pyramid = 0;
    int i = 1;
    while(i <= 5)
    {
      pyramid += i * i;
      i++;
    }
    print(pyramid, '\n');
  }
}
