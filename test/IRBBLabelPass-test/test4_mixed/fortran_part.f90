! SPDX-License-Identifier: BSD-3-Clause
! Copyright (c) 2026 Zhantong Qiu
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!
! 1. Redistributions of source code must retain the above copyright notice, this
!    list of conditions and the following disclaimer.
!
! 2. Redistributions in binary form must reproduce the above copyright notice,
!    this list of conditions and the following disclaimer in the documentation
!    and/or other materials provided with the distribution.
!
! 3. Neither the name of the copyright holder nor the names of its
!    contributors may be used to endorse or promote products derived
!    from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
! DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
! SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
! CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
! OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!
! Test Case 4 - Fortran Part: Fortran subroutines for C++ interoperability
!
! Purpose: Provide Fortran subroutines callable from C++ to test multi-language
! LLVM module instrumentation.
!
! This file demonstrates:
!   - bind(C) attribute for C-compatible function names
!   - iso_c_binding module for C interoperability types
!   - Intent specifications (in/out) for parameter passing
!   - Fortran DO loops compiled to LLVM basic blocks
!
! These subroutines are:
!   1. Compiled to LLVM IR (.ll) using flang-new
!   2. Linked with C++ IR using llvm-link
!   3. Called from C++ via extern "C" declarations
!
! Expected behavior:
!   - Subroutines appear in merged IR with C-compatible names
!   - IRBBLabel pass instruments Fortran basic blocks
!   - CSV includes Fortran functions alongside C++ functions

! Simple Fortran addition subroutine.
!
! Args:
!   a, b: Input integers to add (intent(in))
!   result: Output sum (intent(out))
!
! The bind(C) attribute exports this with C linkage as "fortran_add".
! C++ code can call this via: extern "C" void fortran_add(int,int,int*)
!
! Expected basic blocks:
!   - entry: perform addition and return
subroutine fortran_add(a, b, result) bind(C, name="fortran_add")
  use iso_c_binding
  implicit none
  integer(c_int), intent(in) :: a, b
  integer(c_int), intent(out) :: result
  result = a + b
end subroutine fortran_add

! Fortran factorial with DO loop.
!
! Args:
!   n: Input integer to compute factorial of (intent(in))
!   result: Output factorial value (intent(out))
!
! This subroutine demonstrates Fortran loop constructs which compile
! to LLVM basic blocks (loop header, body, increment, exit).
!
! The DO loop structure:
!   do i = 2, n
!     result = result * i
!   end do
!
! Expected basic blocks:
!   - entry: initialize result = 1
!   - loop header: check if i <= n
!   - loop body: multiply result * i
!   - loop increment: increment i
!   - loop exit: return
subroutine fortran_factorial(n, result) bind(C, name="fortran_factorial")
  use iso_c_binding
  implicit none
  integer(c_int), intent(in) :: n
  integer(c_int), intent(out) :: result
  integer :: i
  
  result = 1
  do i = 2, n
    result = result * i
  end do
end subroutine fortran_factorial
