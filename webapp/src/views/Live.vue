<template>
  <v-container  v-if="$vuetify.breakpoint.lgAndUp && !loading" fill-height>
    <v-layout text-center>
      <v-flex>
        <v-card >
          <v-card-title primary-title class="justify-center">Live View</v-card-title>
          <v-card-text>
            <v-row justify="center" dense>
                <v-col cols=8>
                    <v-select
                    v-model="selectedData"
                    :items="Array.from(availableData.keys())"
                    :menu-props="{ maxHeight: '400' }"
                    label="Select"
                    multiple
                    chips
                    hint="Pick data nodes for display"
                    persistent-hint
                    ></v-select>
                </v-col>
            </v-row>
            <v-row justify="center" dense>
                <v-spacer></v-spacer>
                <v-col cols=8>
                    <v-btn @click="clearSelection()">Clear Selection</v-btn></v-col>
                <v-spacer></v-spacer>
            </v-row>

            <v-row justify="center" class="flex-grow-0">
                <v-col cols=12 >
                  <line-chart :chart-data="storedata" :height="180" :responsive=false :options="options"></line-chart>
                </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
  <v-container  v-else-if="$vuetify.breakpoint.mdAndDown && !loading" fluid fill-height pa-0 ma-0>
    <v-layout text-center align-center>
      <v-flex>
        <div>
          <v-select
            v-model="selectedData"
            :items="$store.state.chartDataKeys"
            :menu-props="{ maxHeight: '400' }"
            label="Select"
            multiple
            outlined
            dense
            hint="Pick data nodes for display"
            persistent-hint
            ></v-select>
            <v-btn @click="clearSelection()">Clear Selection</v-btn>
          <line-chart :chart-data="storedata" :responsive=true :options="options"></line-chart>
        </div>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
import LineChart from '../components/LineChart'

const chartColors = {
	red: 'rgb(255, 99, 132)',
	orange: 'rgb(255, 159, 64)',
	yellow: 'rgb(255, 205, 86)',
	green: 'rgb(75, 192, 192)',
	blue: 'rgb(54, 162, 235)',
	purple: 'rgb(153, 102, 255)'
};


export default {
  components: {
    LineChart
  },
  data () {
    return {
      selectedData: [],
      availableData: null,
      loading: true,
      storedata: null,
      labels: [],
      run: true,
      timer: null,
      intervall: 3,
      numDataPoints: 100,
      //options to pass to chart object
      options: {
        scales: {
            xAxes: [{
              display: true,
              scaleLabel: {
                display: true,
                labelString: 'Minutes elapsed',
                fontSize: 18
              },
              ticks: {
                fontSize: 16
            }
            }],
            yAxes: [{
              ticks: {
                fontSize: 16
            }
            }]
        }
      }
    }
  },
  mounted() {
    //wait for data
    this.$store.dispatch('initChartData').then(() =>{
      clearInterval(this.timer);
      this.timer = setInterval(this.getData, this.intervall*1000);
      this.makeLabels();
      this.createSelection();
    });
  },
  beforeDestroy() {
    clearInterval(this.timer);
  },
  methods: {
    clearSelection() {
      this.selectedData = []
    },
    updateGraph() {
      let data = []
      this.selectedData.forEach((key, index) => {
        //the key from thingset response
        let originalKey = this.availableData.get(key);
        let buf = new Object();
        let colorNames = Object.keys(chartColors);
        buf["label"] = key
        buf["fill"] = false
        buf["pointRadius"] = 2
        buf["lineTension"] = 0
        buf["borderColor"] = chartColors[colorNames[index % colorNames.length]]
        buf["data"] = this.$store.state.chartData[originalKey]
        data.push(buf)
      })
      this.storedata = {
        labels: this.labels,
        datasets: data
      }
    },
    makeLabels() {
      this.labels = Array(this.numDataPoints)
      for(var i = -this.numDataPoints; i != 1; i++) {
          this.labels[this.numDataPoints + i] = "" + (i / 20)
      }
    },
    //necessary to handle cases when info.json could not be loaded
    createSelection() {
      let id = this.$store.state.activeDeviceId
      this.availableData = new Map()
      let key = ""
      this.$store.state.chartDataKeys.forEach((elem) => {
        if(this.$store.state.thingsetStrings[id]?.output[elem]) {
          key = this.$store.state.thingsetStrings[id].output[elem].title.en
        } else {
          key = elem
        }
          this.availableData.set(key, elem)
        })
      this.loading = false
    },
    getData () {
      this.$store.dispatch('updateChartData').then(() => {
        this.updateGraph();
      });
    }
  }
}
</script>