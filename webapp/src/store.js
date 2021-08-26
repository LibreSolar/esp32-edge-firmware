import Vue from 'vue'
import Vuex from 'vuex'
import axios from 'axios'
import info from "../info.json"

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    chartData: {},
    chartDataKeys: [],
    loading: true,
    self: {},
    selfInfo: null,
    devices: {},
    activeDevice: "No Devices connected..",
    activeDeviceId: "",
    info: {},
    thingsetStrings: {},
    isAuthenticated: false,
    globAlert: false,
    globAlertType: "warning",
    globAlertMsg: ""
  },
  mutations: {
    changeDevice(state, key) {
      state.activeDevice = key
      state.activeDeviceId = state.devices[key]
      if (!state.info[key])
        this.dispatch("getDeviceInfo").then(() => {
          this.dispatch("getThingsetStrings")
        })
    },
    saveDevices(state, devices) {
      state.self = devices['self']
      delete devices['self']
      this.dispatch('getInfoSelf')
      if (Object.keys(devices).length > 0) {
        state.devices = devices
        this.commit('changeDevice', Object.keys(devices)[0])
        state.loading = false
      }
    },
    saveDeviceInfo(state, deviceInfo) {
      state.info[state.activeDeviceId] = deviceInfo
    },
    saveThingsetStrings(state, strings) {
      state.thingsetStrings[state.activeDeviceId] = strings
    },
    initChartData(state, newData) {
      const keys = Object.keys(newData)
      keys.forEach(key => {
        newData[key] = Array(180)
        state.chartData = newData
      })
      state.chartDataKeys = keys
    },
    updateChartData(state, newData) {
      state.chartDataKeys.forEach(key => {
        state.chartData[key].shift()
        state.chartData[key].push(newData[key])
      });
    },
    saveAuthStatus(state, status) {
      state.isAuthenticated = status;
    },
    saveInfo(state, data) {
      state.selfInfo = data
    },
    triggerAlert(state, msg) {
        state.globAlert = true
        state.globAlertMsg = msg
        state.globAlertType = "warning"
    },
    resetAlert(state) {
      state.globAlert = false
    }
  },
  actions: {
    authenticate( { commit }, password){
      return axios.post("ts/" + this.state.activeDeviceId + "/auth",
      '"' + password + '"',
      {headers: {"Content-Type": "text/plain"}})
      .then(res => {
        commit('saveAuthStatus', true);
      })
      .catch(error => {
        commit('saveAuthStatus', false);
        console.log(error);
      })
    },
    getDevices( { commit }) {
      return  axios.get('ts/')
        .then(res => {
          if (res.data) {
            commit('saveDevices', res.data)
        }
      }).catch(error => {
        console.log(error);
      })
    },
    getDeviceInfo( { commit }) {
        return axios.get("ts/" + this.state.activeDeviceId + "/info")
        .then(res => {
          commit("saveDeviceInfo", res.data)
        })
        .catch(error => {
          throw error
        });
    },
    getThingsetStrings( { commit }) {
      axios.get(this.state.info[this.state.activeDeviceId].DataExtURL)
      .then(res => {
        commit("saveThingsetStrings", res.data)
      })
      .catch(error => {
        commit("triggerAlert",
              "Unable to fetch extended information for " + this.state.activeDevice +
              " device data. Using basic information discovered from device.")
      })
    },
    initChartData( { commit }) {
      return axios.get("ts/" + this.state.activeDeviceId + "/meas")
        .then(res => {
          commit("initChartData", res.data);
        })
        .catch(error => {
          console.log(error);
        })
    },
    updateChartData({ commit }) {
      return axios.get("ts/" + this.state.activeDeviceId + "/meas")
        .then(res => {
          commit("updateChartData", res.data);
        })
        .catch(error => {
          console.log(error);
        });
    },
    getInfoSelf({ commit }) {
      return axios.get("ts/" + this.state.self + "/info")
        .then(res => {
          commit("saveInfo", res.data);
        }).catch(error => {
          console.log(error);
        })
    },
    async createPrettyStrings({ commit }, section) {
      let id = this.state.activeDeviceId
      if (this.state.thingsetStrings[id]) {
        return this.state.thingsetStrings[id][section]
      } else {
        return {}
      }
    }
  }
})
