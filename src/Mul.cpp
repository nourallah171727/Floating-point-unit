#include "../include/Mul.hpp"

// Algorithm reference :https://www.geeksforgeeks.org/digital-logic/multiplying-floating-point-numbers/


//Initialising the constructor
Mul::Mul(sc_module_name name,uint32_t mantissa_bitsp,uint32_t exponent_bitsp,uint32_t round_modep)
    :sc_module(name),mantissa_bits(mantissa_bitsp),exponent_bits(exponent_bitsp),round_mode(round_modep)
{
    bias =(1<<(exponent_bits-1))-1;
    SC_METHOD(run);
	sensitive<< r1 << r2;
}

void Mul::run(){
    //RUN the Module
     extract();
     if(!handleSpecialCases()){
        multiply();
     }
}


void Mul::multiply(){

    //Multiply the two numbers
    bool is_exact;
        
    // 0.Get the sign of the result
    uint32_t signr= signa ^ signb;

    // 1. Find the actual exponents while handling very small numbers (Stored Exp=0)
    uint32_t stored_e1 =exp1, stored_e2=exp2;
    int32_t e1, e2;//Actual exp
    uint64_t m1=0, m2=0;
        
    if(stored_e1==0){
        // subnormal: no implicit one, exponent = 1-bias-mantissa_bits
        e1= 1-bias-static_cast<int32_t>(mantissa_bits);
        m1= mantissa1;
    }else{
        e1= static_cast<int32_t>(stored_e1)-bias;
        m1= (1ULL << mantissa_bits) | mantissa1;
    }

    if(stored_e2==0){
        e2= 1-bias-static_cast<int32_t>(mantissa_bits);
        m2= mantissa2;
    }else{
        e2= static_cast<int32_t>(stored_e2)-bias;
        m2= (1ULL << mantissa_bits)|mantissa2;
    }

    //2.Multiply + round (may adjust e1, e2)
    uint32_t mantissa_result=  multiply_and_round(m1,m2,e1,e2,is_exact);

    //3.Recompute biased exponent
    int32_t expr=e1+e2;
    int32_t final_exp =expr + bias;
    uint32_t max_field =(1U << exponent_bits)-1;

    //4.Find the final result

    //4.1 Handle Overflow to infinity

    if(final_exp >= static_cast<int32_t>(max_field)){
        //Write Inf
        uint32_t ans =(signr << 31)|(max_field << mantissa_bits);
        ro.write(ans);
        zero.write(false);
        overflow.write(true);
        underflow.write(false);
        inexact.write(!is_exact);
        nan.write(false);
        return;
    }

    //4.2 Underflow to subnormal or zero (Honestly I got some Help from AI)
    if(final_exp<=0){
        int shift = 1-final_exp;

        // start from rounded significand (implicit 1 + frac)
        uint64_t full_sig = ((1ULL << mantissa_bits) | mantissa_result);
        bool sticky = false;

        if(shift <= static_cast<int>(mantissa_bits)+1){
            uint64_t mask =(1ULL << shift)-1;
            sticky = (full_sig & mask) != 0;
            full_sig >>= shift;
        }else{
            sticky = (full_sig != 0);
            full_sig = 0;
        }

        uint32_t sub_mant = static_cast<uint32_t>(full_sig) & ((1U << mantissa_bits) - 1);
        uint32_t ans = (signr << 31) | sub_mant;

        ro.write(ans);
        zero.write(sub_mant==0);
        overflow.write(false);
        underflow.write(true);
        inexact.write(!is_exact || sticky);
        nan.write(false);
        return;
    }

    //4.3 Normal result
    uint32_t ans =(signr << 31)|(static_cast<uint32_t>(final_exp) << mantissa_bits)|mantissa_result;

    //5. Write final signals
     ro.write(ans);
     zero.write(false);
     overflow.write(false);
     underflow.write(false);
     inexact.write(!is_exact);
     nan.write(false);
}

uint32_t Mul::multiply_and_round(uint64_t m1,uint64_t m2,int32_t &e1,int32_t &e2, bool &is_exact){

    //Multiply and round the result depending on Exps and Mantisses

    //2.1 Multiply the Mantisses
    uint64_t product = m1 * m2;

    // 2.2 Check for normalization: if the top bit(one above 2*M bits) is set,
    //     shift mantissa right by 1 and increment exponent e1
    bool norm= product & (1ULL << (2 * mantissa_bits + 1));

    if(norm) e1+=1;
    int drop= norm ? (mantissa_bits+1) : mantissa_bits;

    //2.3 ROUND the Result depending on the product of the mantisses

    //2.3.1 Extract low,guard and stikcy bits:
    uint64_t low = (1ULL << drop) - 1;
    uint32_t guard = (product >> (drop-1)) & 1; //the bit just above the retained mantissa
    uint32_t roundb= (product >> (drop-2)) & 1; //the next bit after guard
    uint32_t sticky = ((product & (low>>2)) != 0) ? 1 : 0; //OR of all lower bits (indicates any non-zero bits were dropped)

    //2.3.2 Find if the result is exact,if not round depending on the round type
    is_exact = ((product & low) == 0);
    uint64_t sig = product >> drop;
    bool inc = false;
    bool signr = signa ^ signb;

    switch(round_mode){
        case 0:
            if(guard && (roundb || sticky)) inc = true;
            else if(guard && !roundb && !sticky && (sig & 1)) inc = true;
            break;
        case 1:
            inc=guard;
            break;
        case 2:
            inc =false;
            break;
        case 3:
            inc= guard && !signr;
             break;
        case 4:
            inc = guard && signr;
            break;
    }

    // 2.3.3 If rounding up, add 1 to the significand
    if(inc){
        sig+=1;
        is_exact = false;
        if(sig & (1ULL << (mantissa_bits+1))){
            sig >>= 1;
            e1+=1;
        }
    }
    // 2.4 Mask out the final mantissa bits and return
    return static_cast<uint32_t>(sig & ((1U<<mantissa_bits)-1));
}


void Mul::extract(){
    //Extract Signs, Exps and Mantisses
    uint32_t v1 =r1.read();
    uint32_t v2 =r2.read();

    signa =(v1 >> 31) & 1;
    signb =(v2 >> 31) & 1;
    exp1 =(v1 >> mantissa_bits) & ((1U << exponent_bits) - 1);
    exp2 =(v2 >> mantissa_bits) & ((1U << exponent_bits) - 1);
    mantissa1 = v1 & ((1U << mantissa_bits) - 1);
    mantissa2 = v2 & ((1U << mantissa_bits) - 1);
}


bool Mul::handleSpecialCases(){
    //Verify special cases regarding Nan Inf and 0
    bool zero1 =(exp1==0 && mantissa1==0);
    bool zero2 =(exp2==0 && mantissa2==0);
    bool inf1 =(exp1==((1U<<exponent_bits)-1) && mantissa1==0);
    bool inf2 =(exp2==((1U<<exponent_bits)-1) && mantissa2==0);
    bool nan1 =(exp1==((1U<<exponent_bits)-1) && mantissa1!=0);
    bool nan2 =(exp2==((1U<<exponent_bits)-1) && mantissa2!=0);

    if(nan1||nan2){
        if((zero1&&nan2)||(zero2&&nan1)){
            //Nan*0=0
            ro.write((signa^signb)<<31);
            zero.write(true);overflow.write(false);underflow.write(false);
            inexact.write(false); nan.write(false);
        }else writeNaN(); //Nan*a= Nan
        return true;
    }
    if(inf1||inf2){
        if((inf1 && zero2)||(inf2 && zero1)) writeNaN(); //Inf * 0= nan
        else{
            //Inf * a= Inf
            uint32_t ans = ((signa^signb)<<31)|(((1U<<exponent_bits)-1)<<mantissa_bits);
            ro.write(ans);
            zero.write(false); overflow.write(true);
            underflow.write(false); inexact.write(false); nan.write(false);
        }
        return true;
    }
    if(zero1||zero2){
        // a*0=0
        ro.write((signa^signb)<<31);
        zero.write(true); overflow.write(false);
        underflow.write(false); inexact.write(false); nan.write(false);
        return true;
    }
    return false;
}

void Mul::writeNaN(){
    //set flags to Nan
    uint32_t ans =(((1U<<exponent_bits)-1)<<mantissa_bits)|(1U<<(mantissa_bits-1));
    ro.write(ans);
    zero.write(false);overflow.write(false);underflow.write(false);
    inexact.write(true);nan.write(true);
}