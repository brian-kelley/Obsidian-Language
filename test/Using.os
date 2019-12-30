module A
{
  proc doAThing: void()
  {
    print("Doing A thing.\n");
  }
}

module B
{
  using module A;
  proc doBThing: void()
  {
    print("Doing B thing.\n");
  }
  proc doAinBThing: void()
  {
    print("Doing A thing in B:\n  ");
    doAThing();
  }
}

proc main: void()
{
  using A.doAThing;
  using module B;
  doAThing();
  doBThing();
  doAinBThing();
}
