typedef (double, double, double) vec3;

func sqHypot: double(v: vec3)
{
  return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

proc main: void()
{
  print("Hypot^2 of 1,2,3: ", sqHypot([1, 2, 3]), '\n');
}
