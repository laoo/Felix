#pragma once

class Cartridge
{
public:
  Cartridge();

  bool getAudIn() const;
  void setAudIn( bool value );

  void setCartAddressData( bool value );
  void setPower( bool value );


};
