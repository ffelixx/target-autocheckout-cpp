const fs = require('fs');
const colors = require('colors');
const httpsProxyAgent = require('https-proxy-agent');
const {
    webhook,
} = require('../config.json')
const profiles = require('../profile.json')
const axios = require('axios')
const {
    Webhook,
    MessageBuilder
} = require('discord-webhook-node');
const hook = new Webhook(webhook);

const proxies = [];
const tasks = []
fs.readFile('./txt/proxies.txt', 'utf-8', async function (err, data) {
    const datas = data.split("\r\n")
    datas.forEach(line => {
        proxies.push(line)
    })
    fs.readFile('./txt/tasks.txt', 'utf-8', async function (err, data) {
        const datas = data.split("\r\n")
        datas.forEach(line => {
            tasks.push(line)
        })
        for (taske of tasks) {
            const proxy = proxies[Math.floor(Math.random() * proxies.length)];
            const index = proxies.indexOf(proxy)
            proxies.splice(index, 1)
            const split = taske.split("\t")
            new Task(split[1], proxy, split[2], split[3], split[4], split[5])
        }
    })
})

class Task {
    constructor(pid, proxy, quantity, profile, delay, cookie) {
        this.config = {
            "pid": pid,
            "quantity": quantity,
            "profile": profile,
            "delay": delay,
            "cookie": cookie
        }
        this.format(proxy)
    }

    async format(proxy) {
        this.split = proxy.split(":")
        const profile = profiles.filter(element => element.name == this.config.profile)
        this.name = profile[0].name
        this.agent = new httpsProxyAgent(`http://${this.split[2]}:${this.split[3]}@${this.split[0]}:${this.split[1]}`);
        this.address_line1 = profile[0].shippingAddress.line1;
        this.address_line2 = profile[0].shippingAddress.line2;
        this.city = profile[0].shippingAddress.city;
        this.first_name = profile[0].shippingAddress.name.split(" ")[0];
        this.last_name = profile[0].shippingAddress.name.split(" ")[1];
        this.mobile = profile[0].shippingAddress.phone;
        this.state = profile[0].shippingAddress.state;
        this.zip_code = profile[0].shippingAddress.postCode;
        this.card_name = profile[0].paymentDetails.nameOnCard;
        this.card_number = profile[0].paymentDetails.cardNumber;
        this.cvv = profile[0].paymentDetails.cardCvv;
        this.expiry_month = profile[0].paymentDetails.cardExpMonth;
        this.expiry_year = profile[0].paymentDetails.cardExpYear;
        this.getCart()
    }

    async getCart() {
        axios({
                method: 'put',
                httpsAgent: this.agent,
                url: 'https://carts.target.com/web_checkouts/v1/cart?field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CPROMOTION_CODES%2CADDRESSES%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39',
                headers: {
                    'authority': 'carts.target.com',
                    'accept': 'application/json',
                    'pragma': 'no-cache',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'content-type': 'application/json',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-cart',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                data: JSON.stringify({
                    "cart_type": "REGULAR",
                    "channel_id": 10,
                    "shopping_context": "DIGITAL"
                })
            })
            .then(async response => {
                if (response.data.cart_items !== undefined) {
                    await response.data.cart_items.forEach(async item => {
                        await this.clearItem(item.cart_item_id)
                    })
                    setTimeout(function () {
                        this.monitor()
                    }.bind(this), 500)
                } else {
                    setTimeout(function () {
                        this.monitor()
                    }.bind(this), 100)
                }
            })
            .catch(async error => {
                if (error.response.data.message == "Unauthorized" || error.response.data.errors[0].errorKey == "ERR_UNAUTHORIZED") {
                    console.log(`Invalid Cookie`.red)
                }
            });
    }

    async clearItem(id) {
        axios({
                method: 'delete',
                httpsAgent: this.agent,
                url: `https://carts.target.com/web_checkouts/v1/cart_items/${id}?cart_type=REGULAR&field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CPROMOTION_CODES%2CADDRESSES%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39`,
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-cart',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                }
            })
            .catch(async error => {
                console.log("Error clearing cart".red);
            });
    }

    async monitor() {
        axios({
                method: 'get',
                httpsAgent: this.agent,
                url: `https://redsky.target.com/redsky_aggregations/v1/web/pdp_fulfillment_v1?key=ff457966e64d5e877fdbad070f276d18ecec4a01&tcin=${this.config.pid}&store_id=3200&store_positions_store_id=3200&has_store_positions_store_id=true&zip=55414&state=NY&latitude=44.9778&longitude=93.2650&scheduled_delivery_store_id=3200&pricing_store_id=3200&has_pricing_store_id=true`,
                headers: {
                    'authority': 'redsky.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': `https://www.target.com/p/-/-/A-${this.config.pid}`,
                    'accept-language': 'en-US,en;q=0.9'
                }
            })
            .then(async response => {
                if (response.data.data.product.fulfillment.shipping_options.availability_status == 'IN_STOCK') {
                    setTimeout(function () {
                        this.addToCart()
                    }.bind(this), 250)
                } else {
                    console.log(`Waiting For Restock`.yellow)
                    setTimeout(function () {
                        this.monitor()
                    }.bind(this), this.config.delay)
                }
            })
            .catch(async error => {
                console.log(`Waiting For Product`.yellow)
                setTimeout(function () {
                    this.monitor()
                }.bind(this), this.config.delay)
            });
    }

    async addToCart() {
        console.log(`Adding To Cart`.yellow)
        axios({
                method: 'post',
                httpsAgent: this.agent,
                url: 'https://carts.target.com/web_checkouts/v1/cart_items?field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39',
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': `https://www.target.com/p/-/-/A-${this.config.pid}`,
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                withCredentials: true,
                data: JSON.stringify({
                    "cart_type": "REGULAR",
                    "channel_id": "90",
                    "shopping_context": "DIGITAL",
                    "cart_item": {
                        "tcin": this.config.pid,
                        "quantity": this.config.quantity,
                        "item_channel_id": "10"
                    }
                })
            })
            .then(async response => {
                if (response.data.inventory_info.availability_status == 'IN_STOCK') {
                    console.log(`Added To Cart`.green)
                    setTimeout(function () {
                        this.loadCheckout()
                    }.bind(this), 500)
                }
            })
            .catch(async error => {
                if (error.response.status == 401) {
                    console.log(`Expired Cookie`.red)
                } else if (error.response.data.alerts[0].message.startsWith('Maximum purchase limit of the item is reached')) {
                    console.log(`Error Adding To Cart - Max Quantity Exceeded`.red)
                } else {
                    console.log(`Error Adding To Cart`.red)
                    setTimeout(function () {
                        this.addToCart()
                    }.bind(this), this.config.delay)
                }
            })
    }

    async loadCheckout() {
        console.log(`Loading Checkout`.yellow)
        axios({
                method: 'post',
                httpsAgent: this.agent,
                url: 'https://carts.target.com/web_checkouts/v1/pre_checkout?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CDELIVERY_WINDOWS%2CPAYMENT_INSTRUCTIONS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39',
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-review?precheckout=true',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                withCredentials: true,
                data: JSON.stringify({
                    "cart_type": "REGULAR"
                })
            })
            .then(async response => {
                try {
                    this.title = response.data.cart_items[0].item_attributes.description
                    this.image = response.data.cart_items[0].item_attributes.image_path
                    this.cart_id = response.data.cart_id
                } catch {
                    this.title = undefined
                    this.image = undefined
                }
                try {
                    this.address_id = response.data.cart_items[0].fulfillment.address_id
                } catch {
                    this.address_id = undefined
                }
                try {
                    this.payment_instruction_id = response.data.payment_instructions[0].payment_instruction_id
                } catch {
                    this.payment_instruction_id = undefined
                }
                console.log(`Loaded Checkout`.green)
                setTimeout(function () {
                    this.submitShipping()
                }.bind(this), 500)
            })
            .catch(async error => {
                console.log(`Error Loading Checkout`.red)
            });
    }

    async submitShipping() {
        if (this.address_id !== undefined) {
            this.addressMethod = "PUT"
            this.addressUrl = `https://carts.target.com/web_checkouts/v1/cart_shipping_addresses/${this.address_id}?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39`
        } else if (this.address_id == undefined) {
            this.addressMethod = "POST"
            this.addressUrl = `https://carts.target.com/web_checkouts/v1/cart_shipping_addresses?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39`
        }
        console.log(`Submitting Shipping`.yellow)
        axios({
                method: this.addressMethod,
                httpsAgent: this.agent,
                url: this.addressUrl,
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-delivery',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                withCredentials: true,
                data: JSON.stringify({
                    "cart_type": "REGULAR",
                    "address": {
                        "address_line1": this.address_line1,
                        "address_line2": this.address_line2,
                        "address_type": "SHIPPING",
                        "city": this.city,
                        "country": "US",
                        "first_name": this.first_name,
                        "last_name": this.last_name,
                        "mobile": this.mobile,
                        "save_as_default": false,
                        "state": this.state,
                        "zip_code": this.zip_code
                    },
                    "selected": true,
                    "save_to_profile": false,
                    "skip_verification": true
                })
            })
            .then(async response => {
                console.log(`Submitted Shipping`.green)
                setTimeout(function () {
                    this.submitPayment()
                }.bind(this), 250)
            })
            .catch(async error => {
                console.log(`Error Submitting Shipping`.red)
            });
    }

    async submitPayment() {
        if (this.payment_instruction_id !== undefined) {
            this.paymentMethod = "PUT"
            this.paymentUrl = `https://carts.target.com/checkout_payments/v1/payment_instructions/${this.payment_instruction_id}?key=feaf228eb2777fd3eee0fd5192ae7107d6224b39`
        }
        if (this.payment_instruction_id == undefined) {
            this.paymentMethod = "POST"
            this.paymentUrl = `https://carts.target.com/checkout_payments/v1/payment_instructions?key=feaf228eb2777fd3eee0fd5192ae7107d6224b39`
        }
        console.log(`Submitting Payment`.yellow)
        axios({
                method: this.paymentMethod,
                httpsAgent: this.agent,
                url: this.paymentUrl,
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-payment',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                data: JSON.stringify({
                    "cart_id": this.cart_id,
                    "wallet_mode": "ADD",
                    "payment_type": "CARD",
                    "card_details": {
                        "card_name": this.card_name,
                        "card_number": this.card_number,
                        "cvv": this.cvv,
                        "expiry_month": this.expiry_month,
                        "expiry_year": this.expiry_year
                    },
                    "billing_address": {
                        "address_line1": this.address_line1,
                        "address_line2": this.address_line2,
                        "city": this.city,
                        "first_name": this.first_name,
                        "last_name": this.last_name,
                        "phone": this.mobile,
                        "state": this.state,
                        "zip_code": this.zip_code,
                        "country": "US"
                    },
                    "skip_verification": true
                })
            })
            .then(async response => {
                this.price = response.data.payment_instruction_amount;
                console.log(`Submitted Payment`.green)
                setTimeout(function () {
                    this.submitOrder()
                }.bind(this), 250)
            })
            .catch(async error => {
                console.log(`Error Submitting Payment`.red)
            });
    }

    async submitOrder() {
        console.log(`Submitting Order`.yellow)
        axios({
                httpsAgent: this.agent,
                method: 'post',
                url: 'https://carts.target.com/web_checkouts/v1/checkout?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CDELIVERY_WINDOWS%2CPAYMENT_INSTRUCTIONS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39',
                headers: {
                    'authority': 'carts.target.com',
                    'sec-ch-ua': '" Not;A Brand";v="99", "Google Chrome";v="91", "Chromium";v="91"',
                    'accept': 'application/json',
                    'x-application-name': 'web',
                    'sec-ch-ua-mobile': '?0',
                    'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36',
                    'content-type': 'application/json',
                    'origin': 'https://www.target.com',
                    'sec-fetch-site': 'same-site',
                    'sec-fetch-mode': 'cors',
                    'sec-fetch-dest': 'empty',
                    'referer': 'https://www.target.com/co-review',
                    'accept-language': 'en-US,en;q=0.9',
                    'cookie': this.config.cookie
                },
                data: JSON.stringify({
                    "cart_type": "REGULAR",
                    "channel_id": 10
                })
            })
            .then(async response => {
                if (response.data.orders[0].cart_state == 'COMPLETED') {
                    console.log(`Checkout Successful`.green)
                    const embed = new MessageBuilder()
                        .setTitle(`Checkout Successful`)
                        .setURL(`https://www.target.com/p/-/-/A-${this.config.pid}`)
                        .addField(`Product`, this.title)
                        .addField(`PID`, this.config.pid)
                        .addField(`Store`, `Target`)
                        .addField(`Price`, `$${this.price.toString()}`)
                        .addField(`Speed`, "1")
                        .addField(`Quantity`, `${this.config.quantity.toString()}`)
                        .addField(`Profile`, `||${this.name}||`)
                        .addField(`Proxy`, `||${this.split[0]}:${this.split[1]}||`)
                        .addField(`Order #`, `||${response.data.orders[0].parent_cart_number}||`)
                        .setColor(`#14dc54`)
                        .setThumbnail(image)
                        .setFooter(`bot aio`)
                        .setTimestamp()
                    hook.send(embed);
                }
            })
            .catch(async error => {
                if (error.response.data.alerts[0].message == 'Payment declined') {
                    console.log(`Payment Declined`.red)
                    const embed = new MessageBuilder()
                        .setTitle(`Payment Declined`)
                        .setURL(`https://www.target.com/p/-/-/A-${this.config.pid}`)
                        .addField(`Product`, this.title)
                        .addField(`PID`, this.config.pid)
                        .addField(`Store`, `Target`)
                        .addField(`Price`, `$${this.price.toString()}`)
                        .addField(`Speed`, "1")
                        .addField(`Quantity`, this.config.quantity.toString())
                        .addField(`Profile`, `||${this.name}||`)
                        .addField(`Proxy`, `||${this.split[0]}:${this.split[1]}||`)
                        .setColor(`#cb260d`)
                        .setThumbnail(this.iamge)
                        .setFooter(`bot aio`)
                        .setTimestamp();
                    hook.send(embed);
                } else {
                    console.log(error.response.data)
                    console.log(`Error Submtiting Order`.red)
                }
            });
    }
}