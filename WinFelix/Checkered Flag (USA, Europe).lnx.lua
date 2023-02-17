ram.x[0x200] = function( value )
  set_label( 0x0305, 'hereherehereherehre' );
  set_label( 0x003b, 'hghgfhfghjgfhjdgfhj' );
  set_label( 0x033e, 'hghtytydgfhj' );
  set_label( 0x0342, 'here' );
  set_label( 0xfe00, 'nothere' );
  return value;
end

