int func4(int x, int esi = 0, int edx = 14) {
	int eax = edx - esi; // 14
	int ecx = eax >> 31; // 0
	eax += ecx; // 14
	eax >>= 1; // 7
	ecx = eax + esi // 7
	if (ecx <= x)
		goto 36;
	edx = ecx - 1; // 6
	func4(x, esi, edx);
	eax *= 2;
	goto 57;
36:
	eax = 0;
	if (ecx >= x)
		goto 57;
	esi = ecx + 1;
	func4(x, esi, edx);
57:
	return 2*eax+1;
}

int func4(int x, int l, int r) {
	int mid = (l + r) / 2;
	if (x >= mid) {
		if (x <= mid) return 0;
		return 2 * func4(x, mid + 1, r) + 1;
	} else {
		return func4(x, l, mid - 1);
	}
}
