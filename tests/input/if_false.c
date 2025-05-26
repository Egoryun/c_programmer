int main() {
    int x;
    x = 0;
    if (1 == 0) {
        x = 10;
    }
    return x; // Expected: 0
}
