    subroutine MPP_DO_GLOBAL_FIELD_3Dnew_( local, global, d_comm )
!get a global field from a local field
!local field may be on compute OR data domain
      type(DomainCommunicator2D), intent(in) :: d_comm
      MPP_TYPE_,                 intent(in)  ::  local(:,:,:)
      MPP_TYPE_, intent(out) :: global(d_comm%domain%x(1)%global%begin:,d_comm%domain%y(1)%global%begin:,:)

      integer :: i, j, k, m, n, nd, nwords, msgsize
      integer :: is, ie, js, je
      MPP_TYPE_ :: clocal ((d_comm%domain%x(1)%compute%size)*    (d_comm%domain%y(1)%compute%size)*    size(local,3))
      MPP_TYPE_ :: cremote((d_comm%domain%x(1)%compute%max_size)*(d_comm%domain%y(1)%compute%max_size)*size(local,3))
      integer :: stackuse
      character(len=8) :: text
      pointer( ptr_local,  clocal  )
      pointer( ptr_remote, cremote )

      ptr_local  = LOC(mpp_domains_stack)
      ptr_remote = LOC(mpp_domains_stack(size(clocal(:))+1))

#ifdef use_CRI_pointers
      stackuse = size(clocal(:))+size(cremote(:))
      if( stackuse.GT.mpp_domains_stack_size )then
          write( text, '(i8)' )stackuse
          call mpp_error( FATAL, &
               'MPP_UPDATE_DOMAINS user stack overflow: call mpp_domains_set_stack_size('//trim(text)//') from all PEs.' )
      end if
      mpp_domains_stack_hwm = max( mpp_domains_stack_hwm, stackuse )
#endif

! make contiguous array from compute domain
      m = 0
      is=d_comm%sendis(1,0); ie=d_comm%sendie(1,0)
      js=d_comm%sendjs(1,0); je=d_comm%sendje(1,0)
      do k = 1,size(local,3)
         do j = js, je
            do i = is, ie
               m = m + 1
               clocal(m) = local(i+d_comm%gf_ioff,j+d_comm%gf_joff,k)
               global(i,j,k) = clocal(m) !always fill local domain directly
            end do
         end do
      end do

! if there is more than one tile on this pe, then no decomposition for all tiles on this pe, so we can just return
      if(size(d_comm%domain%x(:))>1) return

      nd = d_comm%Rlist_size  ! same as size of send list
      msgsize = m  ! constant for all sends
      do n = 1,nd-1
         call mpp_send( clocal(1), plen=msgsize, to_pe=d_comm%cto_pe(n) )
      end do
      do n = 1,nd-1
         nwords = d_comm%R_msize(n)
         call mpp_recv( cremote(1), glen=nwords, from_pe=d_comm%cfrom_pe(n) )
         is=d_comm%recvis(1,n); ie=d_comm%recvie(1,n)
         js=d_comm%recvjs(1,n); je=d_comm%recvje(1,n)
         m = 0
         do k = 1,size(global,3)
            do j = js,je
               do i = is,ie
                  m = m + 1
                  global(i,j,k) = cremote(m)
               end do
            end do
         end do
      end do
      call mpp_sync_self( )
    end subroutine MPP_DO_GLOBAL_FIELD_3Dnew_