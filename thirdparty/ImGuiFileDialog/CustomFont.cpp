#include "CustomFont.h"

static const char FONT_ICON_BUFFER_NAME_IGFD[3105+1] =
    "7])#######qgGmo'/###V),##+Sl##Q6>##w#S+Hh=?<a7*&T&d.7m/oJ[^IflZg#BfG<-iNE/1-2JuBw0'B)i,>>#'tEn/<_[FHkp#L#,)m<-:qEn/@d@UCGD7s$_gG<-]rK8/XU#[A"
    ">7X*M^iEuLQaX1DIMr62DXe(#=eR%#_AFmBFF1J5h@6gLYwG`-77LkOETt?0(MiSAq@ClLS[bfL)YZ##E)1w--Aa+MNq;?#-D^w'0bR5'Cv9N(f$IP/371^#IhOSMoH<mL6kSG2mEexF"
    "TP'##NjToIm3.AF4@;=-1`/,M)F5gL':#gLIlJGMIfG<-IT*COI=-##.<;qMTl:$#L7cwL#3#&#W(^w5i*l.q3;02qtKJ5q'>b9qx:`?q*R[SqOim4vII3L,J-eL,o[njJ@Ro?93VtA#"
    "n;4L#?C(DNgJG&#D;B;-KEII-O&Ys1,AP##0Mc##4Yu##fRXgLC;E$#@(V$#jDA]%f,);?o[3YckaZfCj>)Mp64YS76`JYYZGUSItO,AtgC_Y#>v&##je+gL)SL*57*vM($i?X-,BG`a"
    "*HWf#Ybf;-?G^##$VG13%&>uuYg8e$UNc##D####,03/MbPMG)@f)T/^b%T%S2`^#J:8Y.pV*i(T=)?#h-[guT#9iu&](?#A]wG;[Dm]uB*07QK(]qFV=fV$H[`V$#kUK#$8^fLmw@8%"
    "P92^uJ98=/+@u2OFC.o`5BOojOps+/q=110[YEt$bcx5#kN:tLmb]s$wkH>#iE(E#VA@r%Mep$-#b?1,G2J1,mQOhuvne.Mv?75/Huiw'6`?$=VC]&,EH*7M:9s%,:7QVQM]X-?^10ip"
    "&ExrQF7$##lnr?#&r&t%NEE/2XB+a4?c=?/^0Xp%kH$IM$?YCjwpRX:CNNjL<b7:.rH75/1uO]unpchLY.s%,R=q9V%M,)#%)###Tx^##x@PS.kN%##AFDZ#5D-W.-kr.0oUFb35IL,3"
    "%A2*/RtC.3j85L#gJ))3rx3I):2Cv-FX(9/vBo8%=[%?5BKc8/t=r$#<MfD*.gB.*Q&WS7#r&ipf9b^EhD]:/%A@`aU5b'O]I03N2cUN0ebYF':?AE+OaUq).]tfd]s0$d0C9E#enTtQ"
    "&(oqL^)sB#`:5Yu9A;TFN%MeMBhZd3J0(6:8mc0CdpC,)#5>X(3^*rMo&Y9%)[[c;QIt<1q3n0#MI`QjeUB,MCG#&#n#]I*/=_hLfM]s$Cr&t%#M,W-d9'hlM2'J35i4f)_Y_Z-Mx;=."
    "Z&f.:a[xw'2q'Y$G38$5Zs<7.;G(<-$87V?M42X-n+[w'1q-[Bv(ofL2@R2L/%dDE=?CG)bhho.&1(a4D/NF3;G`=.Su$s$]WD.3jY5lLBMuM(Hnr?#FTFo26N.)*,/_Yox3prHH]G>u"
    "?#Ke$+tG;%M)H3uH@ta$/$bku/1.E4:po2/Z5wGE^e*:*Mgj8&=B]'/-=h1B-n4GVaVQu.gm^6X,&Gj/Run+M,jd##kJ,/1=-U,2QNv)42.,Q'iUKF*wCXI)+f1B4./.&4maJX-'$fF4"
    "uX@8%5Y,>#r:N&l,=GNGA5AZ$XA`0(Q^(k'(GB.WKx+M/mi5###%D1Mh;BE+O.1w,p8QSV`E3$%sMwH)/t6iLF.'DX[G&8Remo7/,w/RNRPUV$)8[0#G,>>#d=OZ-$_Aj0^ll%0uAOZ6"
    "m('J3*`4-6Rq@.*G7K,3L1O058b4-5mS4'5841'5%J/GV/1:B#'at%$q^DIM=X0DMg*^fL6)0/Ldbb(NRdimM-a4o7qPa>$cMDX:W<=&5t^Dv$)2mJ)lb+Ze`eHQ%:oSfLiMK/L@i6o7"
    ":fKb@'x_5/Q1wPMIR0cMmbR`aRLb>-w..e-R3n0#bsn`$bA%%#N,>>#MhSM'Z`qdm?D,c4A]DD3mMWB#,5Rv$g&?a3?9wGMeANv>s#V&1iJ&%bE2ZA#-.^g1j;R)4443%b7RU`u`Kk(N"
    "HbeV@gJS'-BWcP3m?+#-VaDuL:`WY5kEaau^=lJ(_24J-WvW+.VxIfLXqd##Q3=&#d72mLvG(u$+dfF4<BPF%MPEb3:]Me.d(4I)(P;e.)_K#$^n;#MjXGp%jDJeM20P.)'9_-2[x):8"
    ",VjfLQJa0V*#4o7dD24'^bO-)GX3/U@@%P'9/8b*;XmGA9Gw;8i=I`(sZhM'8A]?cof-6M>Awx-tS<GMRoS+MWWB@#WG9J'P@)?uNUYI8#-EE#[Yw_#;R,<8+b?:3=t$gLFf]0#G?O&#"
    "DoA*#_>QJ(PGpkL'(MB#/T01YP6;hLeb6lL7$(f):3ou-KHaJM[TXD#'1;p72$[p.Z9OA#RcB##Oi./LlA)ZuTBqn'B]7%b/WM<LrFcS7XOtILd9b<UxX'^#)J/Dt]il3++^tpAu_L%,"
    "w$[gu5[-['#**&+*wwGXO4h=#%H3'50S6##l9f/)m`):)t1@k=?\?#]u3i8-#%/5##)]$s$+JNh#jl###cU^F*Vs'Y$Lov[-0<a?$Hni?#+?2?up1m%.*%%-NPB`>$agNe'Qk[X'ep.'5"
    "8=B_A+L1_A'fp`Nae&%#aKb&#W>gkLZ/(p$V2Cv-_uv20tt?X-BL75/#(KU)N0;hLr75c4br9s-;va.3exLG`^U;4F9D+tqKKGSIULS:d=vRduSxXCuOq$0ufu'L#>Y[OVk1k3G(kZoA"
    "O9iQN'h5',b_mL,v?qr&4uG##%J/GV5J?`a'=@@M#w-tL0+xRV6,A48_fa-Zups+;(=rhZ;ktD#c9OA#axJ+*?7%s$DXI5/CKU:%eQ+,2=C587Rg;E4Z3f.*?ulW-tYqw0`EmS/si+C#"
    "[<;S&mInS%FAov#Fu[E+*L'O'GWuN'+)U40`a5k'sA;)*RIRF%Tw/Z--1=e?5;1X:;vPk&CdNjL*(KJ1/,TV-G(^S*v14gL#8,,M*YPgLaII@b+s[&#n+d3#1jk$#';P>#,Gc>#0Su>#"
    "4`1?#8lC?#<xU?#@.i?#D:%@#i'LVCn)fQD[.C(%ea@uBoF/ZGrDFVCjZ/NB0)61Fg&cF$v:7FHFDRb3bW<2Be(&ZG17O(Ie4;hFwf1eGEmqA4sS4VCoC%eG1PM*HsH%'I]MBnD+'],M"
    "w)n,Ga8q`EgKJ.#wZ/x=v`#/#";
