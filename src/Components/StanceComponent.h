#pragma once
enum class Stance { HoldFire,
                    FireAtWill };
struct StanceComponent {
    Stance stance = Stance::FireAtWill;
};