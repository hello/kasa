package ExtUtils::CBuilder::Platform::linux;

use strict;
use ExtUtils::CBuilder::Platform::Unix;
use File::Spec;

use vars qw($VERSION @ISA);
$VERSION = '0.280206';
@ISA = qw(ExtUtils::CBuilder::Platform::Unix);

sub link {
  my ($self, %args) = @_;
  my $cf = $self->{config};

  # Link XS modules to libperl.so explicitly because multiple
  # dlopen(, RTLD_LOCAL) hides libperl symbols from XS module.
  local $cf->{lddlflags} = $cf->{lddlflags};
  if ($ENV{PERL_CORE}) {
    $cf->{lddlflags} .= ' -L' . $self->perl_inc();
  }
  $cf->{lddlflags} .= ' -lperl';

  return $self->SUPER::link(%args);
}

1;
