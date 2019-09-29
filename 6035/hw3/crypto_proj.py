import hashlib
import json
import math
import os
import random

class CryptoProject(object):

    def __init__(self):
        self.student_id = 'bdornier3'

    def get_student_id_hash(self):
        return hashlib.sha224(self.student_id.encode('UTF-8')).hexdigest()

    def get_all_data_from_json(self, filename):
        data = None
        base_dir = os.path.abspath(os.path.dirname(__file__))
        with open(os.path.join(base_dir, filename), 'r') as f:
            data = json.load(f)
        return data

    def get_data_from_json_for_student(self, filename):
        data = self.get_all_data_from_json(filename)
        name = self.get_student_id_hash()
        if name not in data:
            print(self.student_id + ' not in file ' + filename)
            return None
        else:
            return data[name]

    def mod_exponent(self, n, e, m):
        """
        Computes modular exponent using repeated square method.
        """

        if e < 0:
            raise Exception("exponent should be greater than pr equal to 0")

        r = 1 % m
        while e > 0:
            if e & 1:
                r = (r * n) % m

            n = (n * n) % m
            e = e >> 1

        return r

    def totient_from_primes(self, p1, p2):
        """
        Computes totient of no n resulted after multiplying 2 primes p1 and p2 (n = p1.p2).
        It is assumed both p1 and p2 are prime but no check is performed to ensure it.
        """

        return (p1 - 1) * (p2 - 1)

    def extended_gcd(self, a, b):
        """
        Recursively computes gcd using extended Euclidean algorithm.
        If a, b are the 2 no such that (a < b), find gcd(a,b) using below:
        a.x1 + b.y1 = (b.mod a).x2 + a.y2 = gcd(a, b)
        where,
        x2 = y1 - floor(b/a).x1
        y2 = x1

        The algorithm terminates when a' == 0 and b' as required gcd.
        At termination:
        0.xn + b'.yn = gcd(a, b)
        here, xn = 0, b' = gcd(a,b), yn = 1
        """

        if a == 0:
            return b, 0, 1
        else:
            g, x, y = self.extended_gcd(b % a, a)
            return g, y - (b // a) * x, x

    def mod_inverse(self, a, m):
        """
        Computes mod inverse using extended gcd algo.
        On termination, extended gcd returns:
        a.x + b.Y = gcd(a, b)

        inv(a) mod m is defined iff a and m are relatively prime i.e. gcd(a, m) = 1
        Above equation becomes:
        a.x + m.y = gcd(a, m) = 1
        a.x mod m = 1
        x = inv(a) mod m
        """

        g, x, _ = self.extended_gcd(a, m)
        if g == 1:
            return (x + m) % m

    def int_root(self, n, r):
        """
        Computes rth root of n using binary search.
        Works iff, rth root of n is a positive integer >= 1.
        """

        if n < 1 or r < 1:
            raise Exception("both n and r should be >= 1")

        h, l = n, 1
        while h > l:
            m = (h + l) // 2
            p = m ** r
            if p == n:
                return m
            elif p > n:
                h = m - 1
            else:
                l = m + 1

    def decrypt_message(self, N, e, d, c):
        """
        m = c^d mod N
        """

        m = self.mod_exponent(c, d, N)

        return hex(m).rstrip('L')

    def crack_password_hash(self, password_hash, weak_password_list):
        """
        Iterates over all paris in the given list of passwords and
        returns the pair whose hash mathes the hash being searched.
        """

        for i in range(1, len(weak_password_list)):
            for j in range(0, i):
                p1 = weak_password_list[i]
                p2 = weak_password_list[j]
                ep1 = p1.encode()
                ep2 = p2.encode()
                if password_hash == hashlib.sha256(ep1 + ep2).hexdigest():
                    return p1, p2
                if password_hash == hashlib.sha256(ep2 + ep1).hexdigest():
                    return p2, p1

    def get_factors(self, n):
        """
        Computes 2 factors (need not be prime) of the given no n.
        In this context, n is just a special case (product of 2 primes).
        """

        if n % 2 == 0:
            return 2, n // 2

        p = int(n ** (1 / 2))
        if p * p == n:
            return p, p

        if p % 2 == 0:
            p = p - 1

        while p > 0:
            q = n // p
            if p * q == n:
                return p, q

            p = p - 2

    def get_private_key_from_p_q_e(self, p, q, e):
        """
        d = inv(e) mod phi(p.q)
        """

        d = self.mod_inverse(e, self.totient_from_primes(p, q))

        return d

    def is_waldo(self, n1, n2):
        """
        Finds if n1 and n2 has a common factor.
        If so, there gcd won't be equal to 1.
        """

        g, _, _ = self.extended_gcd(n1, n2)
        if g != 1:
            return True

        return False

    def get_private_key_from_n1_n2_e(self, n1, n2, e):
        """
        Finds the private key corresponding to public key (e, n1), given that gcd(n1, n2) > 1.
        p = g = gcd(n1, n2)
        q = n1 / p
        d = inv(e) mod phi(p.q)
        """

        g, _, _ = self.extended_gcd(n1, n2)

        return self.get_private_key_from_p_q_e(g, n1 // g, e)

    def recover_msg(self, N1, N2, N3, C1, C2, C3):
        """
        3 different attack vectors are considered:
        1. Low exponent vulnerability i.e.
           if C1 == C2 or C2 == C3 or C1 == C3,
           then it would mean m = eth_root of corresponding Cn.
        2. Same as get_private_key_from_n1_n2_e, where a pairs on N share a root.
        3. Broadcast attack with a low exponent i.e.
           C1 = m**e mod N1
           C2 = m**e mod N2
           C3 = m**e mod N3
           Implies =>
           m**e = N1*i + C1
           m**e = N2*j + C1
           m**e = N3*k + C1
           Where i, j and k are integers
           Implies =>
           m**e = C1 mod N1
           m**e = C2 mod N2
           m**e = C3 mod N3
           By Chinese reminder theorem =>
           m**e = (C1.N2.N3.(inv(N2.N3) mod N1) +
                C2.N1.N3.(inv(N1.N3) mod N2) +
                C3.N1.N2.(inv(N1.N2) mod N3)) mod N1.N2.N3
           m = eth_root(m**e)
        """

        e = 3

        if C1 == C2 and N1 != N2:
            return self.int_root(C1, e)

        if C2 == C3 and N2 != N3:
            return self.int_root(C2, e)

        if C1 == C3 and N1 != N3:
            return self.int_root(C1, e)

        g, _, _ = self.extended_gcd(N1, N2)
        if g != 1:
            d = self.get_private_key_from_n1_n2_e(N1, N2, e)
            return self.mod_exponent(C1, d, N1)

        g, _, _ = self.extended_gcd(N2, N3)
        if g != 1:
            d = self.get_private_key_from_n1_n2_e(N2, N3, e)
            return self.mod_exponent(C2, d, N1)

        g, _, _ = self.extended_gcd(N1, N3)
        if g != 1:
            d = self.get_private_key_from_n1_n2_e(N1, N3, e)
            return self.mod_exponent(C1, d, N1)

        N12 = N1 * N2
        N23 = N2 * N3
        N13 = N1 * N3
        N123 = N1 * N2 * N3
        m3 = ((((((C1 * N23) % N123) * self.mod_inverse(N23, N1)) % N123) +
            ((((C2 * N13) % N123) * self.mod_inverse(N13, N2)) % N123)) % N123 +
            ((((C3 * N12) % N123) * self.mod_inverse(N12, N3)) % N123)) % N123

        return self.int_root(m3, e)

    def task_1(self):
        data = self.get_data_from_json_for_student('keys4student_task_1.json')
        N = int(data['N'], 16)
        e = int(data['e'], 16)
        d = int(data['d'], 16)
        c = int(data['c'], 16)

        m = self.decrypt_message(N, e, d, c)
        return m

    def task_2(self):
        data = self.get_data_from_json_for_student('hashes4student_task_2.json')
        password_hash = data['password_hash']

        # The password file is loaded as a convenience
        weak_password_list = []
        base_dir = os.path.abspath(os.path.dirname(__file__))
        with open(os.path.join(base_dir, 'top_passwords.txt'), 'r', encoding='UTF-8-SIG') as f:
            pw = f.readline()
            while pw:
                weak_password_list.append(pw.strip('\n'))
                pw = f.readline()

        password, salt = self.crack_password_hash(password_hash, weak_password_list)

        return password, salt

    def task_3(self):
        data = self.get_data_from_json_for_student('keys4student_task_3.json')
        n = int(data['N'], 16)
        e = int(data['e'], 16)

        p, q = self.get_factors(n)
        d = self.get_private_key_from_p_q_e(p, q, e)

        return hex(d).rstrip('L')

    def task_4(self):
        all_data = self.get_all_data_from_json('keys4student_task_4.json')
        student_data = self.get_data_from_json_for_student('keys4student_task_4.json')
        n1 = int(student_data['N'], 16)
        e = int(student_data['e'], 16)

        d = 0
        waldo = 'Dolores'

        for classmate in all_data:
            if classmate == self.get_student_id_hash():
                continue
            n2 = int(all_data[classmate]['N'], 16)

            if self.is_waldo(n1, n2):
                waldo = classmate
                d = self.get_private_key_from_n1_n2_e(n1, n2, e)
                break

        return hex(d).rstrip("L"), waldo

    def task_5(self):
        data = self.get_data_from_json_for_student('keys4student_task_5.json')
        N1 = int(data['N0'], 16)
        N2 = int(data['N1'], 16)
        N3 = int(data['N2'], 16)
        C1 = int(data['C0'], 16)
        C2 = int(data['C1'], 16)
        C3 = int(data['C2'], 16)

        m = self.recover_msg(N1, N2, N3, C1, C2, C3)
        # Convert the int to a message string
        msg = bytes.fromhex(hex(m).rstrip('L')[2:]).decode('UTF-8')

        return msg
