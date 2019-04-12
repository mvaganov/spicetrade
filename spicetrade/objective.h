struct Objective
{
    int points;
    int bonusPoints; // for the gold or silver coin
    const char * resourceCost;
    Objective(const char * resourceCost, int points)
        :resourceCost(resourceCost),points(points),bonusPoints(0){}
};