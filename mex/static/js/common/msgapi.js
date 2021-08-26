/**
 * Message APIs with javascript postmessage (XDM)
 * @author  Byunghun Hwang <bh.hwang@iae.re.kr>
 * @brief   message interfaces between ui window instances
 * @note    
 */


/* Cross Domain Messaging API call */
function xdm_call(func, args){
    window.parent.postMessage({'function':func, 'args':args}, '*'); //'{{frontend.target_origin}}'
}