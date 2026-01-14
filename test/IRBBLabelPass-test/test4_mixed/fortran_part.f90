! Test Case 4 - Fortran Part: Fortran subroutines for C++ interoperability
! Purpose: Test mixed C++ and Fortran in single LLVM module

! Simple Fortran addition subroutine
subroutine fortran_add(a, b, result) bind(C, name="fortran_add")
    use iso_c_binding
    implicit none
    integer(c_int), intent(in) :: a, b
    integer(c_int), intent(out) :: result
    result = a + b
end subroutine fortran_add

! Fortran factorial with loop
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
