typedef (double, double, double) vec3;
func double sqHypot(vec3 v)
{
  return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

proc void main()
{
  print("Hypot^2 of 1,2,3: ", sqHypot([1, 2, 3]), '\n');
}
